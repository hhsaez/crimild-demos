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

class ExampleVulkanSystem : public GLFWVulkanSystem {
public:
    crimild::Bool start( void ) override
    {
        if ( !GLFWVulkanSystem::start() ) {
            return false;
        }

        m_frameGraph = crimild::alloc< FrameGraph >();

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
                    auto &a = ( int ) 11;
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

        if ( m_frameGraph->compile() ) {
            auto commands = m_frameGraph->recordCommands();
            setCommandBuffers( { commands } );
        }

        return true;
    }

    void update( void ) override
    {
        auto clock = Simulation::getInstance()->getSimulationClock();
        m_scene->perform( UpdateComponents( clock ) );
        m_scene->perform( UpdateWorldState() );

        GLFWVulkanSystem::update();
    }

    void stop( void ) override
    {
        if ( auto renderDevice = getRenderDevice() ) {
            renderDevice->waitIdle();
        }

        m_scene = nullptr;
        m_renderPass = nullptr;
        m_master = nullptr;
        m_frameGraph = nullptr;

        GLFWVulkanSystem::stop();
    }

private:
    SharedPointer< Node > m_scene;
    SharedPointer< FrameGraph > m_frameGraph;
    SharedPointer< RenderPass > m_renderPass;
    SharedPointer< PresentationMaster > m_master;
};

int main( int argc, char **argv )
{
    crimild::init();
    crimild::vulkan::init();

    Log::setLevel( Log::Level::LOG_LEVEL_ALL );

    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "Triangle", crimild::alloc< Settings >( argc, argv ) );
    sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );
    return sim->run();
}
