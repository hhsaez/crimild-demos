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
#include <Crimild_Vulkan.hpp>
#include <Crimild_GLFW.hpp>
#include <Crimild_STB.hpp>

namespace crimild {

    /*

    class LightingShaderProgram : public ShaderProgram {
    public:
        LightingShaderProgram( std::string name )
        {
            auto createShader = []( Shader::Stage stage, std::string path ) {
                return crimild::alloc< Shader >(
                    stage,
                    FileSystem::getInstance().readFile(
                        FilePath {
                            .path = path,
                        }.getAbsolutePath()
                    )
                );
            };

            setShaders(
                Array< SharedPointer< Shader >> {
                    createShader(
                        Shader::Stage::VERTEX,
                        "assets/shaders/" + name + ".vert.spv"
                    ),
                    createShader(
                        Shader::Stage::FRAGMENT,
                        "assets/shaders/" + name + ".frag.spv"
                    ),
                    }
            );

            vertexLayouts = { VertexP3N3::getLayout() };

            descriptorSetLayouts = {
                [] {
                    auto layout = crimild::alloc< DescriptorSetLayout >();
                    layout->bindings = {
                        {
                            .descriptorType = DescriptorType::UNIFORM_BUFFER,
                            .stage = Shader::Stage::VERTEX,
                        },
                        {
                            .descriptorType = DescriptorType::UNIFORM_BUFFER,
                            .stage = Shader::Stage::ALL,
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
        }

        virtual ~LightingShaderProgram( void ) = default;
    };

    class LightingPipeline : public Pipeline {
    public:
        virtual ~LightingPipeline( void ) = default;
    };

    class LightingMaterial : public Material {
    public:
        virtual ~LightingMaterial( void ) = default;
    };

    */

}

using namespace crimild;
using namespace crimild::glfw;

class ExampleVulkanSystem : public GLFWVulkanSystem {
public:
    crimild::Bool start( void ) override
    {
        if ( !GLFWVulkanSystem::start() ) {
            return false;
        }

		m_frameGraph = crimild::alloc< FrameGraph >();

        m_scene = [&] {
            auto scene = crimild::alloc< Group >();

            auto createMaterial = []( auto program ) {
                auto material = crimild::alloc< SimpleLitMaterial >();
                material->getPipeline()->program = crimild::retain( program );
                material->setDiffuse( RGBAColorf( 1.0f, 0.1f, 0.1f, 1.0f ) );
                return material;
            };

            auto gouraudMaterial = createMaterial( AssetManager::getInstance()->get< GouraudLitShaderProgram >() );
            auto phongMaterial = createMaterial( AssetManager::getInstance()->get< PhongLitShaderProgram >() );

            auto sphere = []( auto position, auto material ) {
                auto geometry = crimild::alloc< Geometry >();
                geometry->attachPrimitive(
                    crimild::alloc< SpherePrimitive >(
                        SpherePrimitive::Params {
                            .type = Primitive::Type::TRIANGLES,
                            .layout = VertexP3N3TC2::getLayout(),
                        }
                    )
                );
                geometry->attachComponent< MaterialComponent >()->attachMaterial( material );
                geometry->local().setTranslate( position );
                return geometry;
            };

            scene->attachNode( sphere( Vector3f( -1.25f, 0.0f, 0.0f ), gouraudMaterial ) );
            scene->attachNode( sphere( Vector3f( 1.25f, 0.0f, 0.0f ), phongMaterial ) );

            scene->attachNode([] {
                auto camera = crimild::alloc< Camera >();
                camera->local().setTranslate( 0.0f, 0.0f, 5.0f );
                camera->local().lookAt( Vector3f::ZERO );
                Camera::setMainCamera( camera );
                return camera;
            }());

            return scene;
        }();

		m_renderPass = [&] {
            auto renderPass = crimild::alloc< RenderPass >();
            renderPass->attachments = {
                [&] {
                    auto att = crimild::alloc< Attachment >();
                    att->format = Format::COLOR_SWAPCHAIN_OPTIMAL;
                    return att;
                }(),
                [&] {
                    auto att = crimild::alloc< Attachment >();
                    att->format = Format::DEPTH_STENCIL_DEVICE_OPTIMAL;
                    return att;
                }()
            };

            renderPass->setDescriptors(
                [&] {
                    auto descriptorSet = crimild::alloc< DescriptorSet >();
                    descriptorSet->descriptors = {
                        Descriptor {
                            .descriptorType = DescriptorType::UNIFORM_BUFFER,
                            .obj = [&] {
                                FetchCameras fetch;
                                m_scene->perform( fetch );
                                auto camera = fetch.anyCamera();
                                return crimild::alloc< CameraViewProjectionUniform >( camera );
                            }(),
                        },
                        Descriptor {
                            .descriptorType = DescriptorType::UNIFORM_BUFFER,
                            .obj = [&] {
                                return crimild::alloc< UniformBuffer >( Vector4f( 10.0f, 10.0f, 10.0f, 0.0f ) );
                            }(),
                        },
                    };
                    return descriptorSet;
                }()
            );
            renderPass->commands = [&] {
                auto commandBuffer = crimild::alloc< CommandBuffer >();
                m_scene->perform(
                    ApplyToGeometries(
                        [&]( Geometry *g ) {
                            if ( auto ms = g->getComponent< MaterialComponent >() ) {
                                if ( auto material = ms->first() ) {
                                    commandBuffer->bindGraphicsPipeline( material->getPipeline() );
                                    commandBuffer->bindDescriptorSet( renderPass->getDescriptors() );
                                    commandBuffer->bindDescriptorSet( material->getDescriptors() );
                                    commandBuffer->bindDescriptorSet( g->getDescriptors() );
                                    commandBuffer->drawPrimitive( g->anyPrimitive() );
                                }
                            }
						}
					)
				);
				return commandBuffer;
			}();

			return renderPass;
		}();

		m_master = [&] {
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

    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "Lighting: Gouraud vs Phong", crimild::alloc< Settings >( argc, argv ) );

    SharedPointer< ImageManager > imageManager = crimild::alloc< crimild::stb::ImageManager >();

    sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );

    return sim->run();
}
