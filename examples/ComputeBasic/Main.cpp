/*
 * Copyright (c) 2002 - present, H. Hernan Saez
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <Crimild.hpp>
#include <Crimild_GLFW.hpp>
#include <Crimild_Vulkan.hpp>

using namespace crimild;
using namespace crimild::glfw;
using namespace crimild::vulkan;

const auto WIDTH = 10000;
const auto HEIGHT = 10000;
const auto WORKGROUP_SIZE = 32;

struct Pixel {
    float r;
    float g;
    float b;
    float a;
};

class ExampleVulkanSystem : public GLFWVulkanSystem {
public:
    crimild::Bool start( void ) override
    {
        if ( !GLFWVulkanSystem::start() ) {
            return false;
        }

        m_frameGraph = crimild::alloc< FrameGraph >();

        /*
        m_scene = [ & ] {
            auto scene = crimild::alloc< Group >();

            scene->attachNode( [ & ] {
                auto geometry = crimild::alloc< Geometry >();
                geometry->attachPrimitive(
                    [ & ] {
                        auto primitive = crimild::alloc< Primitive >(
                            Primitive::Type::TRIANGLES );
                        primitive->setVertexData(
                            {
                                [ & ] {
                                    return crimild::alloc< VertexBuffer >(
                                        VertexP3C3::getLayout(),
                                        Array< VertexP3C3 > {
                                            {
                                                .position = Vector3f( -0.5f, -0.5f, 0.0f ),
                                                .color = RGBColorf( 1.0f, 0.0f, 0.0f ),
                                            },
                                            {
                                                .position = Vector3f( 0.5f, -0.5f, 0.0f ),
                                                .color = RGBColorf( 0.0f, 1.0f, 0.0f ),
                                            },
                                            {
                                                .position = Vector3f( 0.0f, 0.5f, 0.0f ),
                                                .color = RGBColorf( 0.0f, 0.0f, 1.0f ),
                                            },
                                        } );
                                }(),
                            } );
                        primitive->setIndices(
                            crimild::alloc< IndexBuffer >(
                                Format::INDEX_32_UINT,
                                Array< crimild::UInt32 > {
                                    0,
                                    1,
                                    2,
                                } ) );
                        return primitive;
                    }() );
                return geometry;
            }() );

            scene->attachNode( [] {
                auto camera = crimild::alloc< Camera >();
                camera->local().setTranslate( 0.0f, 0.0f, 3.0f );
                Camera::setMainCamera( camera );
                return camera;
            }() );
            return scene;
        }();
        */

        auto data = Array< RGBAColorf >( WIDTH * HEIGHT ).fill( []( auto i ) { return RGBAColorf( 0.0f, 1.0f, 0.0f, 1.0f ); } );
        m_buffer = crimild::alloc< StorageBuffer >( data ); //Array< Pixel >( WIDTH * HEIGHT ) );

        m_pipeline = [ & ] {
            auto pipeline = crimild::alloc< ComputePipeline >();
            pipeline->setProgram(
                [] {
                    auto program = crimild::alloc< ShaderProgram >(
                        Array< SharedPointer< Shader > > {
                            crimild::alloc< Shader >(
                                Shader::Stage::COMPUTE,
                                R"(
#define WIDTH 10000
#define HEIGHT 10000
#define WORKGROUP_SIZE 32
layout (local_size_x = WORKGROUP_SIZE, local_size_y = WORKGROUP_SIZE, local_size_z = 1 ) in;

struct Pixel{
  vec4 value;
};

layout(std140, binding = 0) buffer buf
{
   Pixel imageData[];
};

                                void main( void )
                                {
/*
  In order to fit the work into workgroups, some unnecessary threads are launched.
  We terminate those threads here.
  */
  if(gl_GlobalInvocationID.x >= WIDTH || gl_GlobalInvocationID.y >= HEIGHT)
    return;

  float x = float(gl_GlobalInvocationID.x) / float(WIDTH);
  float y = float(gl_GlobalInvocationID.y) / float(HEIGHT);

  /*
  What follows is code for rendering the mandelbrot set.
  */
  vec2 uv = vec2(x,y);
  float n = 0.0;
  vec2 c = vec2(-.445, 0.0) +  (uv - 0.5)*(2.0+ 1.7*0.2  ),
  z = vec2(0.0);
  const int M =128;
  for (int i = 0; i<M; i++)
  {
    z = vec2(z.x*z.x - z.y*z.y, 2.*z.x*z.y) + c;
    if (dot(z, z) > 2) break;
    n++;
  }

  // we use a simple cosine palette to determine color:
  // http://iquilezles.org/www/articles/palettes/palettes.htm
  float t = float(n) / float(M);
  vec3 d = vec3(0.3, 0.3 ,0.5);
  vec3 e = vec3(-0.2, -0.3 ,-0.5);
  vec3 f = vec3(2.1, 2.0, 3.0);
  vec3 g = vec3(0.0, 0.1, 0.0);
  vec4 color = vec4( d + e*cos( 6.28318*(f*t+g) ) ,1.0);

  // store the rendered mandelbrot set into a storage buffer:
  imageData[WIDTH * gl_GlobalInvocationID.y + gl_GlobalInvocationID.x].value = color;
                                }
                            )" ),
                        } );
                    program->descriptorSetLayouts = {
                        [] {
                            auto layout = crimild::alloc< DescriptorSetLayout >();
                            layout->bindings = {
                                {
                                    .descriptorType = DescriptorType::STORAGE_BUFFER,
                                    .stage = Shader::Stage::COMPUTE,
                                },
                            };
                            return layout;
                        }(),
                    };
                    return program;
                }() );
            return pipeline;
        }();

        m_descriptors = [ & ] {
            auto ds = crimild::alloc< DescriptorSet >();
            ds->descriptors = {
                Descriptor {
                    .descriptorType = DescriptorType::STORAGE_BUFFER,
                    .obj = m_buffer,
                },
            };
            return ds;
        }();

#if 0
        m_computePass = [ & ] {
            auto pass = crimild::alloc< ComputePass >();
            pass->commands = [ & ] {
                auto commandBuffer = crimild::alloc< CommandBuffer >();

                commandBuffer->bindComputePipeline( crimild::get_ptr( m_computePipeline ) );
                commandBuffer->bindDescriptorSet( crimild::get_ptr( m_descriptors ) );
                commandBuffer->dispatch(
                    DispatchWorkgroup {
                        .x = Numeric< UInt32 >::ceil( Real32( WIDTH ) / Real32( WORKGROUP_SIZE ) ),
                        .y = Numeric< UInt32 >::ceil( Real32( WIDTH ) / Real32( WORKGROUP_SIZE ) ),
                        .z = 1,
                    } );

                return commandBuffer;
            }();

            return pass;
        }();
#else
        auto commandBuffer = crimild::alloc< CommandBuffer >();
        commandBuffer->begin( CommandBuffer::Usage::ONE_TIME_SUBMIT );
        commandBuffer->bindComputePipeline( crimild::get_ptr( m_pipeline ) );
        commandBuffer->bindDescriptorSet( crimild::get_ptr( m_descriptors ) );
        commandBuffer->dispatch(
            DispatchWorkgroup {
                .x = Numeric< UInt32 >::ceil( Real32( WIDTH ) / Real32( WORKGROUP_SIZE ) ),
                .y = Numeric< UInt32 >::ceil( Real32( WIDTH ) / Real32( WORKGROUP_SIZE ) ),
                .z = 1,
            } );
        commandBuffer->end();

        m_frameGraph->compile();

        auto renderDevice = getRenderDevice();
        renderDevice->submitComputeCommands( crimild::get_ptr( commandBuffer ) );
        renderDevice->waitIdle();

        Simulation::getInstance()->stop();
        return true;
#endif

        /*
        m_renderPass = [ & ] {
            auto renderPass = crimild::alloc< RenderPass >();
            renderPass->attachments = {
                [ & ] {
                    auto att = crimild::alloc< Attachment >();
                    att->format = Format::COLOR_SWAPCHAIN_OPTIMAL;
                    return att;
                }()
            };
            renderPass->setPipeline(
                [ & ] {
                    auto pipeline = crimild::alloc< Pipeline >();
                    pipeline->program = [ & ] {
                        auto createShader = []( Shader::Stage stage, std::string path ) {
                            return crimild::alloc< Shader >(
                                stage,
                                FileSystem::getInstance().readFile(
                                    FilePath {
                                        .path = path,
                                    }
                                        .getAbsolutePath() ) );
                        };

                        auto program = crimild::alloc< ShaderProgram >(
                            Array< SharedPointer< Shader > > {
                                createShader(
                                    Shader::Stage::VERTEX,
                                    "assets/shaders/scene.vert.spv" ),
                                createShader(
                                    Shader::Stage::FRAGMENT,
                                    "assets/shaders/scene.frag.spv" ),
                            } );
                        program->vertexLayouts = { VertexLayout::P3_C3 };
                        program->descriptorSetLayouts = {
                            [] {
                                auto layout = crimild::alloc< DescriptorSetLayout >();
                                layout->bindings = {
                                    {
                                        .descriptorType = DescriptorType::UNIFORM_BUFFER,
                                        .stage = Shader::Stage::VERTEX,
                                    },
                                };
                                return layout;
                            }(),
                            [] {
                                auto layout = crimild::alloc< DescriptorSetLayout >();
                                layout->bindings = {
                                    {
                                        .descriptorType = DescriptorType::UNIFORM_BUFFER,
                                        .stage = Shader::Stage::VERTEX,
                                    },
                                };
                                return layout;
                            }(),
                        };
                        return program;
                    }();
                    return pipeline;
                }() );
            renderPass->setDescriptors(
                [ & ] {
                    auto descriptorSet = crimild::alloc< DescriptorSet >();
                    descriptorSet->descriptors = {
                        Descriptor {
                            .descriptorType = DescriptorType::UNIFORM_BUFFER,
                            .obj = [ & ] {
                                FetchCameras fetch;
                                m_scene->perform( fetch );
                                auto camera = fetch.anyCamera();
                                return crimild::alloc< CameraViewProjectionUniform >( camera );
                            }(),
                        },
                    };
                    return descriptorSet;
                }() );
            renderPass->commands = [ & ] {
                auto commandBuffer = crimild::alloc< CommandBuffer >();
                m_scene->perform(
                    ApplyToGeometries(
                        [ & ]( Geometry *g ) {
                            commandBuffer->bindGraphicsPipeline( renderPass->getPipeline() );
                            commandBuffer->bindDescriptorSet( renderPass->getDescriptors() );
                            commandBuffer->bindDescriptorSet( g->getDescriptors() );
                            commandBuffer->bindVertexBuffer( crimild::get_ptr( g->anyPrimitive()->getVertexData()[ 0 ] ) );
                            commandBuffer->bindIndexBuffer( g->anyPrimitive()->getIndices() );
                            commandBuffer->drawIndexed( g->anyPrimitive()->getIndices()->getIndexCount() );
                        } ) );
                return commandBuffer;
            }();

            return renderPass;
        }();

        m_master = [ & ] {
            auto master = crimild::alloc< PresentationMaster >();
            master->colorAttachment = m_renderPass->attachments[ 0 ];
            return master;
        }();

        */

        if ( m_frameGraph->compile() ) {
            auto commands = m_frameGraph->recordCommands();
            setCommandBuffers( { commands } );

            //setComputeCommandBuffers( { m_frameGraph->recordComputeCommands() } );
        }
        return true;
    }

    void update( void ) override
    {
        /*
        auto clock = Simulation::getInstance()->getSimulationClock();
        m_scene->perform( UpdateComponents( clock ) );
        m_scene->perform( UpdateWorldState() );
        */

        GLFWVulkanSystem::update();

        Simulation::getInstance()->stop();
    }

    void stop( void ) override
    {
        if ( auto renderDevice = getRenderDevice() ) {
            renderDevice->waitIdle();
        }

        saveImage();

        //m_scene = nullptr;
        //m_renderPass = nullptr;
        //m_master = nullptr;
        m_frameGraph = nullptr;

        GLFWVulkanSystem::stop();
    }

private:
    void saveImage( void ) noexcept
    {
        auto renderDevice = getRenderDevice();
        if ( renderDevice == nullptr ) {
            return;
        }

        renderDevice->mapFromDevice( crimild::get_ptr( m_buffer ) );

        auto outputPath = FileSystem::getInstance().pathForDocument( "screenshot.ppm" );
        std::ofstream out( outputPath, std::ios::out );

        out << "P3"
            << "\n"
            << int( WIDTH ) << " " << int( HEIGHT )
            << "\n255"
            << "\n";

        std::cout << "\nSaving " << outputPath << std::endl;
        m_buffer->each< RGBAColorf >(
            [ &out ]( const auto &p, auto index ) {
                out << Int32( p.r() * 255.0 ) << " " << Int32( p.g() * 255.0 ) << " " << Int32( p.b() * 255.0 ) << "\n";
            } );
        //        for ( const auto &c : pixels ) {
        //            writeColor( out, c, samplesPerPixel );
        //        }

        //m_buffer->mapMemory();

        // save png
    }

private:
    SharedPointer< FrameGraph > m_frameGraph;
    SharedPointer< StorageBuffer > m_buffer;
    SharedPointer< ComputePipeline > m_pipeline;
    SharedPointer< DescriptorSet > m_descriptors;
    SharedPointer< ComputePass > m_computePass;
};

int main( int argc, char **argv )
{
    crimild::init();
    crimild::vulkan::init();

    Log::setLevel( Log::Level::LOG_LEVEL_ALL );

    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "Compute: Basic", crimild::alloc< Settings >( argc, argv ) );
    sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );
    return sim->run();
}
