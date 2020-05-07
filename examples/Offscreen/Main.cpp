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

		auto sceneProgram = [] {
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
			
			auto program = crimild::alloc< ShaderProgram >(
				containers::Array< SharedPointer< Shader >> {
					createShader(
						Shader::Stage::VERTEX,
						"assets/shaders/scene.vert.spv"
					),
					createShader(
						Shader::Stage::FRAGMENT,
						"assets/shaders/scene.frag.spv"
					),
				}
			);
			program->attributeDescriptions = VertexP3C3TC2::getAttributeDescriptions( 0 );
			program->bindingDescription = VertexP3C3TC2::getBindingDescription( 0 );
			program->descriptorSetLayout = [] {
				auto layout = crimild::alloc< DescriptorSetLayout >();
				layout->bindings = {
					{
						.descriptorType = DescriptorType::UNIFORM_BUFFER,
						.stage = Shader::Stage::VERTEX,
					},
                    {
                        .descriptorType = DescriptorType::UNIFORM_BUFFER,
                        .stage = Shader::Stage::VERTEX,
                    },
					{
						.descriptorType = DescriptorType::COMBINED_IMAGE_SAMPLER,
						.stage = Shader::Stage::FRAGMENT,
					},
				};
				return layout;
			}();
			return program;
		}();

        auto mirrorProgram = [] {
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

            auto program = crimild::alloc< ShaderProgram >(
                containers::Array< SharedPointer< Shader >> {
                    createShader(
                        Shader::Stage::VERTEX,
                        "assets/shaders/mirror.vert.spv"
                    ),
                    createShader(
                        Shader::Stage::FRAGMENT,
                        "assets/shaders/mirror.frag.spv"
                    ),
                }
            );
            program->attributeDescriptions = VertexP3C3TC2::getAttributeDescriptions( 0 );
            program->bindingDescription = VertexP3C3TC2::getBindingDescription( 0 );
            program->descriptorSetLayout = [] {
                auto layout = crimild::alloc< DescriptorSetLayout >();
                layout->bindings = {
                    {
                        .descriptorType = DescriptorType::UNIFORM_BUFFER,
                        .stage = Shader::Stage::VERTEX,
                    },
                    {
                        .descriptorType = DescriptorType::COMBINED_IMAGE_SAMPLER,
                        .stage = Shader::Stage::FRAGMENT,
                    },
                };
                return layout;
            }();
            return program;
        }();

		// TODO: decouple camera uniforms from scene
 		auto createTriangle = [&]( Camera *camera, crimild::Real32 direction ) {
			auto node = crimild::alloc< Node >();

			auto renderable = node->attachComponent< RenderStateComponent >();
			renderable->pipeline = [&] {
				auto pipeline = crimild::alloc< Pipeline >();
                pipeline->program = sceneProgram;
				pipeline->cullFaceState = CullFaceState::DISABLED;
				return pipeline;
			}();
			renderable->vbo = crimild::alloc< VertexP3C3TC2Buffer >(
				containers::Array< VertexP3C3TC2 > {
					{
                        .position = Vector3f( 0.0f, 2.0f, 0.0f ),
						.color = RGBColorf( 1.0f, 0.0f, 0.0f ),
						.texCoord = Vector2f::ZERO,
                    },
					{
						.position = Vector3f( -1.0f, 0.0f, 0.0f ),
						.color = RGBColorf( 0.0f, 1.0f, 0.0f ),
						.texCoord = Vector2f::ZERO,
                    },
					{
						.position = Vector3f( 1.0f, 0.0f, 0.0f ),
						.color = RGBColorf( 0.0f, 0.0f, 1.0f ),
						.texCoord = Vector2f::ZERO,
                    },
                }
			);
			renderable->ibo = crimild::alloc< IndexUInt32Buffer >(
				containers::Array< crimild::UInt32 > {
					0, 1, 2,
                }
			);
			renderable->uniforms = {
				[&] {
					auto ubo = crimild::alloc< ModelViewProjectionUniformBuffer >();
					ubo->node = crimild::get_ptr( node );
					ubo->camera = camera;
					return ubo;
				}(),
                [&] {
                    auto ubo = crimild::alloc< UniformBufferImpl< Matrix4f >>();
                    ubo->setData(
                    	[&] {
                            if ( direction > 0 ) {
                                return Matrix4f::IDENTITY;
                            }
                        	return Matrix4f(
                                1.0f, 0.0f, 0.0f, 0.0f,
                                0.0f, -1.0f, 0.0f, 0.0f,
                                0.0f, 0.0f, 1.0f, 0.0f,
                                0.0f, 0.0f, 0.0f, 1.0f
                            );
                        }()
                    );
                    return ubo;
                }(),
			};
			renderable->textures = {
				[] {
					auto texture = crimild::alloc< Texture >();
					texture->imageView = crimild::alloc< ImageView >();
					texture->imageView->image = Image::ONE;
					texture->sampler = [] {
						auto sampler = crimild::alloc< Sampler >();
						sampler->setMinFilter( Sampler::Filter::NEAREST );
						sampler->setMagFilter( Sampler::Filter::NEAREST );
						return sampler;
					}();
                    return texture;
				}(),
			};
			
			node->attachComponent< RotationComponent >( Vector3f::UNIT_Y, 0.1f );
            node->local().setScale( 2.0f );
			
			return node;
		};

		// TODO: decouple camera uniforms from scene
 		auto createPlane = [&]( Camera *camera, SharedPointer< ImageView > const &imageView ) {
			auto node = crimild::alloc< Node >();

			auto renderable = node->attachComponent< RenderStateComponent >();
			renderable->pipeline = [&] {
				auto pipeline = crimild::alloc< Pipeline >();
                pipeline->program = mirrorProgram;
				pipeline->cullFaceState = CullFaceState::DISABLED;
				return pipeline;
			}();
			renderable->vbo = crimild::alloc< VertexP3C3TC2Buffer >(
				containers::Array< VertexP3C3TC2 > {
                    {
                        .position = Vector3f( -1.33f, 0.0f, -1.0f ),
                        .color = RGBColorf::ONE,
                        .texCoord = Vector2f( 0.0f, 0.0f ),
                    },
                    {
                        .position = Vector3f( -1.33f, 0.0f, 1.0f ),
                        .color = RGBColorf::ONE,
                        .texCoord = Vector2f( 0.0f, 1.0f ),
                    },
                    {
                        .position = Vector3f( 1.33f, 0.0f, 1.0f ),
                        .color = RGBColorf::ONE,
                        .texCoord = Vector2f( 1.0f, 1.0f ),
                    },
                    {
                        .position = Vector3f( 1.33f, 0.0f, -1.0f ),
                        .color = RGBColorf::ONE,
                        .texCoord = Vector2f( 1.0f, 0.0f ),
                    },
            	}
			);
			renderable->ibo = crimild::alloc< IndexUInt32Buffer >(
				containers::Array< crimild::UInt32 > {
					0, 1, 2,
                    0, 2, 3,
                }
			);
			renderable->uniforms = {
				[&] {
					auto ubo = crimild::alloc< ModelViewProjectionUniformBuffer >();
					ubo->node = crimild::get_ptr( node );
					ubo->camera = camera;
					return ubo;
				}(),
			};
			renderable->textures = {
                [&] {
                    auto texture = crimild::alloc< Texture >();
                    texture->imageView = imageView;
                    texture->sampler = [] {
                        auto sampler = crimild::alloc< Sampler >();
                        sampler->setMinFilter( Sampler::Filter::NEAREST );
                        sampler->setMagFilter( Sampler::Filter::NEAREST );
                        return sampler;
                    }();
                    return texture;
                }(),
            };

			return node;
		};

        auto createCamera = [&] {
            auto camera = crimild::alloc< Camera >();
            camera->local().setTranslate( 7.0f, 3.0f, 6.0f );
            camera->local().lookAt( Vector3f::UNIT_Y );
            return camera;
        };

		m_offscreenScene = [&] {
			auto group = crimild::alloc< Group >();

            auto camera = createCamera();
			group->attachNode( camera );
            group->attachNode( createTriangle( crimild::get_ptr( camera ), -1.0f ) );
			return group;
		}();

		auto offscreenImageView = [&] {
			auto color = [&] {
				auto att = m_frameGraph->create< Attachment >();
				att->usage = Attachment::Usage::COLOR_ATTACHMENT;
				att->format = Format::R8G8B8A8_UNORM;
				att->imageView = m_frameGraph->create< ImageView >();
                att->imageView->image = m_frameGraph->create< Image >();
				return att;
			}();
			
			auto renderPass = m_frameGraph->create< RenderPass >();
			renderPass->attachments = { color };
			renderPass->commands = [&] {
				auto commandBuffer = crimild::alloc< CommandBuffer >();
				m_offscreenScene->perform(
					Apply(
						[ commandBuffer ]( Node *node ) {
							if ( auto renderState = node->getComponent< RenderStateComponent >() ) {
								renderState->commandRecorder( crimild::get_ptr( commandBuffer ) );
							}
						}
					)
				);
				return commandBuffer;
			}();
//			renderPass->clearValue = {
//				.color = RGBAColorf( 1.0, 0.0, 0.0, 0.0f ),
//			};
            // TODO: fixme
//            renderPass->viewport = {
//                .scalingMode = ScalingMode::FIXED,
//                .dimensions = Rectf( 0.0f, 0.0f, 512.0f, 512.0f ),
//            };

			return color->imageView;
		}();

		m_scene = [&] {
			auto group = crimild::alloc< Group >();

            auto camera = createCamera();
			
            group->attachNode(
                [&] {
                    auto node = createPlane( crimild::get_ptr( camera ), offscreenImageView );
                    node->local().setScale( 10.0f );
                    return node;
                }()
            );
			group->attachNode(
				[&] {
                    auto node = createTriangle( crimild::get_ptr( camera ), 1.0f );
					return node;
				}()
			);
            group->attachNode( camera );
			return group;
		}();

        {
            auto color = [&] {
                auto att = m_frameGraph->create< Attachment >();
                att->usage = Attachment::Usage::COLOR_ATTACHMENT;
                att->format = Format::COLOR_SWAPCHAIN_OPTIMAL;
                return att;
            }();

            auto renderPass = m_frameGraph->create< RenderPass >();
            renderPass->attachments = { color };
            renderPass->commands = [&] {
                auto commandBuffer = crimild::alloc< CommandBuffer >();
                m_scene->perform(
                    Apply(
                        [ commandBuffer ]( Node *node ) {
                            if ( auto renderState = node->getComponent< RenderStateComponent >() ) {
                                renderState->commandRecorder( crimild::get_ptr( commandBuffer ) );
                            }
                        }
                    )
                );
                return commandBuffer;
            }();
//            renderPass->clearValue = {
//                .color = RGBAColorf( 0.5f, 0.5f, 0.5f, 0.0f ),
//            };

            auto master = m_frameGraph->create< PresentationMaster >();
            master->colorAttachment = color;
        }

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
        m_offscreenScene->perform( UpdateComponents( clock ) );
        m_offscreenScene->perform( UpdateWorldState() );

        GLFWVulkanSystem::update();
    }

    void stop( void ) override
    {
        if ( auto renderDevice = getRenderDevice() ) {
            renderDevice->waitIdle();
        }

        m_scene = nullptr;
        m_offscreenScene = nullptr;
        m_frameGraph = nullptr;

        GLFWVulkanSystem::stop();
    }

private:
    SharedPointer< Node > m_scene;
    SharedPointer< Node > m_offscreenScene;
    SharedPointer< FrameGraph > m_frameGraph;
};

int main( int argc, char **argv )
{
    crimild::init();
    crimild::vulkan::init();

    Log::setLevel( Log::Level::LOG_LEVEL_ALL );

    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "Offscreen", crimild::alloc< Settings >( argc, argv ) );
    sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );
    return sim->run();
}

