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

struct RenderPassUniform {
	Matrix4f view;
	Matrix4f proj;
};

class RenderPassUniformBuffer : public UniformBufferImpl< RenderPassUniform > {
public:
	~RenderPassUniformBuffer( void ) = default;
	
	Camera *camera = nullptr;
	Matrix4f reflect = Matrix4f::IDENTITY;
	
	void updateIfNeeded( void ) noexcept override
	{
		setData({
			.view = [&] {
				if ( camera != nullptr ) {
					// if a camera has been specified, we use that one to get the view matrix
					return reflect * camera->getViewMatrix();
				}
				
				if ( auto camera = Camera::getMainCamera() ) {
					// if no camera has been set, let's use whatever's the main one
					return reflect * camera->getViewMatrix();
				}
				
				// no camera
				return Matrix4f::IDENTITY;
			}(),
			.proj = [&] {
				auto proj = Matrix4f::IDENTITY;
				if ( camera != nullptr ) {
					proj = camera->getProjectionMatrix();
				}
				else if ( auto camera = Camera::getMainCamera() ) {
					proj = camera->getProjectionMatrix();
				}
				return proj;
			}(),
		});
	}
};

struct ModelUniform {
	Matrix4f model;
};

class ModelUniformBuffer : public UniformBufferImpl< ModelUniform > {
public:
	~ModelUniformBuffer( void ) = default;

	Node *node = nullptr;

	void updateIfNeeded( void ) noexcept override
	{
		setData({
			.model = [&] {
				return node != nullptr ? node->getWorld().computeModelMatrix() : Matrix4f::IDENTITY;
			}(),
		});
	}
};

struct Library {
	SharedPointer< FrameGraph > frameGraph;
	SharedPointer< PresentationMaster > master;

	struct Scenes {
        struct Scene {
            SharedPointer< Group > root;
            SharedPointer< Camera > camera;
        };

		Scene scene;
        Scene screen;
	} scenes;
	
	struct Programs {
		SharedPointer< ShaderProgram > scene;
		SharedPointer< ShaderProgram > mirror;
        SharedPointer< ShaderProgram > screen;
	} programs;

	struct Passes {
		struct Pass {
			SharedPointer< RenderPass > renderPass;
			SharedPointer< Attachment > color;
            SharedPointer< DescriptorSet > descriptorSet;
            containers::Array< SharedPointer< UniformBuffer >> uniforms;
		};

		Pass offscreen;
		Pass scene;
        Pass screen;
	} passes;
};

class Reflective : public NodeComponent {
    CRIMILD_IMPLEMENT_RTTI( Reflective )
public:
    ~Reflective( void ) = default;
};

class ExampleVulkanSystem : public GLFWVulkanSystem {
public:
    crimild::Bool start( void ) override
    {
        if ( !GLFWVulkanSystem::start() ) {
            return false;
        }

		m_library.frameGraph = crimild::alloc< FrameGraph >();

		m_library.programs.scene = [] {
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
                        {
                            .descriptorType = DescriptorType::COMBINED_IMAGE_SAMPLER,
                            .stage = Shader::Stage::FRAGMENT,
                        },
                    };
                    return layout;
                }(),
            };
			return program;
		}();

        m_library.programs.mirror = [] {
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
                        {
                            .descriptorType = DescriptorType::COMBINED_IMAGE_SAMPLER,
                            .stage = Shader::Stage::FRAGMENT,
                        },
                    };
                    return layout;
                }(),
            };
            return program;
        }();

        m_library.programs.screen = m_library.programs.scene;

        auto createTriangle = [&] {
			auto node = crimild::alloc< Node >();

			auto renderable = node->attachComponent< RenderStateComponent >();
			renderable->pipeline = [&] {
#pragma mark triangle pipeline
				auto pipeline = crimild::alloc< Pipeline >();
                pipeline->program = m_library.programs.scene;
             	pipeline->descriptorSetLayouts = pipeline->program->descriptorSetLayouts;
                pipeline->attributeDescriptions = pipeline->program->attributeDescriptions;
                pipeline->bindingDescription = pipeline->program->bindingDescription;
				pipeline->cullFaceState = CullFaceState::DISABLED;
                pipeline->viewport = { .scalingMode = ScalingMode::DYNAMIC };
                pipeline->scissor = { .scalingMode = ScalingMode::DYNAMIC };
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
					auto ubo = crimild::alloc< ModelUniformBuffer >();
					ubo->node = crimild::get_ptr( node );
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
			renderable->descriptorSet = [&] {
				auto descriptorSet = crimild::alloc< DescriptorSet >();
				descriptorSet->descriptorSetLayout = m_library.programs.scene->descriptorSetLayouts[ 1 ];
				descriptorSet->descriptorPool = crimild::alloc< DescriptorPool >();
                descriptorSet->descriptorPool->descriptorSetLayout = descriptorSet->descriptorSetLayout;
				descriptorSet->writes = {
					{
						.descriptorType = DescriptorType::UNIFORM_BUFFER,
						.buffer = crimild::get_ptr( renderable->uniforms[ 0 ] ),
					},
					{
						.descriptorType = DescriptorType::COMBINED_IMAGE_SAMPLER,
						.texture = crimild::get_ptr( renderable->textures[ 0 ] ),
					},
				};
				return descriptorSet;
			}();
			
			node->attachComponent< RotationComponent >( Vector3f::UNIT_Y, 0.1f );
            node->local().setScale( 2.0f );
			
			return node;
		};

 		auto createPlane = [&]( SharedPointer< ImageView > const &imageView ) {
			auto node = crimild::alloc< Node >();

			auto renderable = node->attachComponent< RenderStateComponent >();
			renderable->pipeline = [&] {
#pragma mark plane pipieline
				auto pipeline = crimild::alloc< Pipeline >();
                pipeline->program = m_library.programs.mirror;
             	pipeline->descriptorSetLayouts = pipeline->program->descriptorSetLayouts;
                pipeline->attributeDescriptions = pipeline->program->attributeDescriptions;
                pipeline->bindingDescription = pipeline->program->bindingDescription;
				pipeline->cullFaceState = CullFaceState::DISABLED;
                pipeline->viewport = { .scalingMode = ScalingMode::DYNAMIC };
                pipeline->scissor = { .scalingMode = ScalingMode::DYNAMIC };
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
                    auto ubo = crimild::alloc< ModelUniformBuffer >();
                    ubo->node = crimild::get_ptr( node );
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
            renderable->descriptorSet = [&] {
                auto descriptorSet = crimild::alloc< DescriptorSet >();
                descriptorSet->descriptorSetLayout = m_library.programs.scene->descriptorSetLayouts[ 1 ];
                descriptorSet->descriptorPool = crimild::alloc< DescriptorPool >();
                descriptorSet->descriptorPool->descriptorSetLayout = descriptorSet->descriptorSetLayout;
                descriptorSet->writes = {
                    {
                        .descriptorType = DescriptorType::UNIFORM_BUFFER,
                        .buffer = crimild::get_ptr( renderable->uniforms[ 0 ] ),
                    },
                    {
                        .descriptorType = DescriptorType::COMBINED_IMAGE_SAMPLER,
                        .texture = crimild::get_ptr( renderable->textures[ 0 ] ),
                    },
                };
                return descriptorSet;
            }();

            node->attachComponent< Reflective >();

			return node;
		};

        auto createCamera = [&] {
            auto camera = crimild::alloc< Camera >();
            camera->local().setTranslate( 7.0f, 3.0f, 6.0f );
            camera->local().lookAt( Vector3f::UNIT_Y );
            return camera;
        };

		m_library.passes.offscreen.color = [&] {
			auto att = crimild::alloc< Attachment >();
			att->usage = Attachment::Usage::COLOR_ATTACHMENT;
			att->format = Format::R8G8B8A8_UNORM;
			att->imageView = crimild::alloc< ImageView >();
			att->imageView->image = crimild::alloc< Image >();
			return att;
		}();

        m_library.scenes.scene = [&] {
            auto group = crimild::alloc< Group >();

            auto camera = createCamera();

            group->attachNode(
                [&] {
                    auto node = createPlane( m_library.passes.offscreen.color->imageView );
#pragma mark QUAD_SIZE
                node->local().setScale( 10.0f );
                    return node;
                }()
            );
            group->attachNode(
                [&] {
                    auto node = createTriangle();
                    return node;
                }()
            );
            group->attachNode( camera );
            return Library::Scenes::Scene {
                .root = group,
                .camera = camera,
            };
        }();

        m_library.passes.offscreen.uniforms = {
            [&] {
                auto ubo = crimild::alloc< RenderPassUniformBuffer >();
                ubo->camera = crimild::get_ptr( m_library.scenes.scene.camera );
                ubo->reflect = Matrix4f(
                    1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, -1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f
                );
                return ubo;
            }()
        };

        m_library.passes.offscreen.descriptorSet = [&] {
            auto descriptorSet = crimild::alloc< DescriptorSet >();
            descriptorSet->descriptorSetLayout = m_library.programs.scene->descriptorSetLayouts[ 0 ];
            descriptorSet->descriptorPool = crimild::alloc< DescriptorPool >();
            descriptorSet->descriptorPool->descriptorSetLayout = descriptorSet->descriptorSetLayout;
            descriptorSet->writes = {
                {
                    .descriptorType = DescriptorType::UNIFORM_BUFFER,
                    .buffer = crimild::get_ptr( m_library.passes.offscreen.uniforms[ 0 ] ),
                },
            };
            return descriptorSet;
        }();
			
		m_library.passes.offscreen.renderPass = [&] {
			auto renderPass = crimild::alloc< RenderPass >();
			renderPass->attachments = { m_library.passes.offscreen.color };
			renderPass->commands = [&] {
				auto commandBuffer = crimild::alloc< CommandBuffer >();
                auto viewport = ViewportDimensions { .scalingMode = ScalingMode::RELATIVE };
                commandBuffer->setViewport( viewport );
                commandBuffer->setScissor( viewport );
                m_library.scenes.scene.root->perform(
					Apply(
						[&]( Node *node ) {
                    		if ( node->getComponent< Reflective >() != nullptr ) {
                        		return;
                    		}
							if ( auto renderState = node->getComponent< RenderStateComponent >() ) {
                                commandBuffer->bindGraphicsPipeline( crimild::get_ptr( renderState->pipeline ) );
                                commandBuffer->bindVertexBuffer( crimild::get_ptr( renderState->vbo ) );
                                commandBuffer->bindIndexBuffer( crimild::get_ptr( renderState->ibo ) );
                                commandBuffer->bindDescriptorSet( crimild::get_ptr( m_library.passes.offscreen.descriptorSet ) );
                                commandBuffer->bindDescriptorSet( crimild::get_ptr( renderState->descriptorSet ) );
                                commandBuffer->drawIndexed(
                                    renderState->ibo->getCount()
                                );
							}
						}
					)
				);
				return commandBuffer;
			}();
#pragma mark - offscreen viewport
            renderPass->extent = {
                .scalingMode = ScalingMode::FIXED,
                .width = 512,
                .height = 512,
            };
			return renderPass;
		}();
			
        m_library.passes.scene.uniforms = {
            [&] {
                auto ubo = crimild::alloc< RenderPassUniformBuffer >();
                ubo->camera = crimild::get_ptr( m_library.scenes.scene.camera );
                ubo->reflect = Matrix4f::IDENTITY;
                return ubo;
            }()
        };

        m_library.passes.scene.descriptorSet = [&] {
            auto descriptorSet = crimild::alloc< DescriptorSet >();
            descriptorSet->descriptorSetLayout = m_library.programs.scene->descriptorSetLayouts[ 0 ];
            descriptorSet->descriptorPool = crimild::alloc< DescriptorPool >();
            descriptorSet->descriptorPool->descriptorSetLayout = descriptorSet->descriptorSetLayout;
            descriptorSet->writes = {
                {
                    .descriptorType = DescriptorType::UNIFORM_BUFFER,
                    .buffer = crimild::get_ptr( m_library.passes.scene.uniforms[ 0 ] ),
                },
            };
            return descriptorSet;
        }();

		m_library.passes.scene.color = [&] {
			auto att = crimild::alloc< Attachment >();
			att->usage = Attachment::Usage::COLOR_ATTACHMENT;
            att->format = Format::R8G8B8A8_UNORM;
            att->imageView = crimild::alloc< ImageView >();
            att->imageView->image = crimild::alloc< Image >();
			return att;
		}();

#pragma mark - scene renderpass
		m_library.passes.scene.renderPass = [&] {
			auto renderPass = crimild::alloc< RenderPass >();
            renderPass->attachments = { m_library.passes.scene.color };
            renderPass->commands = [&] {
                auto commandBuffer = crimild::alloc< CommandBuffer >();
                auto viewport = ViewportDimensions { .scalingMode = ScalingMode::RELATIVE };
                commandBuffer->setViewport( viewport );
                commandBuffer->setScissor( viewport );
                m_library.scenes.scene.root->perform(
                    Apply(
                        [&]( Node *node ) {
                            if ( auto renderState = node->getComponent< RenderStateComponent >() ) {
                                commandBuffer->bindGraphicsPipeline( crimild::get_ptr( renderState->pipeline ) );
                                commandBuffer->bindVertexBuffer( crimild::get_ptr( renderState->vbo ) );
                                commandBuffer->bindIndexBuffer( crimild::get_ptr( renderState->ibo ) );
                                commandBuffer->bindDescriptorSet( crimild::get_ptr( m_library.passes.scene.descriptorSet ) );
                                commandBuffer->bindDescriptorSet( crimild::get_ptr( renderState->descriptorSet ) );
                                commandBuffer->drawIndexed(
                                    renderState->ibo->getCount()
                                );
                            }
                        }
                    )
                );
                return commandBuffer;
            }();
            renderPass->clearValue = {
                .color = RGBAColorf( 0.5f, 0.5f, 0.5f, 0.0f ),
            };
			return renderPass;
		}();

        m_library.scenes.screen = [&] {
            auto group = crimild::alloc< Group >();

            group->attachNode(
            	[&] {
                    auto node = crimild::alloc< Node >();
                    auto renderable = node->attachComponent< RenderStateComponent >();
                    renderable->pipeline = [&] {
#pragma mark - screen pipeline
                        auto pipeline = crimild::alloc< Pipeline >();
                        pipeline->program = m_library.programs.scene;
                        pipeline->descriptorSetLayouts = pipeline->program->descriptorSetLayouts;
                        pipeline->attributeDescriptions = pipeline->program->attributeDescriptions;
                        pipeline->bindingDescription = pipeline->program->bindingDescription;
                        pipeline->cullFaceState = CullFaceState::DISABLED;
                        return pipeline;
                    }();
                    renderable->vbo = crimild::alloc< VertexP3C3TC2Buffer >(
                        containers::Array< VertexP3C3TC2 > {
                            {
                                .position = Vector3f( -1.0f, 1.0f, 0.0f ),
                                .color = RGBColorf::ONE,
                                .texCoord = Vector2f( 0.0f, 0.0f ),
                            },
                            {
                                .position = Vector3f( -1.0f, -1.0f, 0.0f ),
                                .color = RGBColorf::ONE,
                                .texCoord = Vector2f( 0.0f, 1.0f ),
                            },
                            {
                                .position = Vector3f( 1.0f, -1.0f, 0.0f ),
                                .color = RGBColorf::ONE,
                                .texCoord = Vector2f( 1.0f, 1.0f ),
                            },
                            {
                                .position = Vector3f( 1.0f, 1.0f, 0.0f ),
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
                            auto ubo = crimild::alloc< ModelUniformBuffer >();
                            ubo->node = crimild::get_ptr( node );
                            return ubo;
                        }(),
                    };
                    renderable->textures = {
                        [&] {
                            auto texture = crimild::alloc< Texture >();
                            texture->imageView = m_library.passes.scene.color->imageView;
                            texture->sampler = [] {
                                auto sampler = crimild::alloc< Sampler >();
                                sampler->setMinFilter( Sampler::Filter::NEAREST );
                                sampler->setMagFilter( Sampler::Filter::NEAREST );
                                return sampler;
                            }();
                            return texture;
                        }(),
                    };
                    renderable->descriptorSet = [&] {
                        auto descriptorSet = crimild::alloc< DescriptorSet >();
                        descriptorSet->descriptorSetLayout = m_library.programs.scene->descriptorSetLayouts[ 1 ];
                        descriptorSet->descriptorPool = crimild::alloc< DescriptorPool >();
                        descriptorSet->descriptorPool->descriptorSetLayout = descriptorSet->descriptorSetLayout;
                        descriptorSet->writes = {
                            {
                                .descriptorType = DescriptorType::UNIFORM_BUFFER,
                                .buffer = crimild::get_ptr( renderable->uniforms[ 0 ] ),
                            },
                            {
                                .descriptorType = DescriptorType::COMBINED_IMAGE_SAMPLER,
                                .texture = crimild::get_ptr( renderable->textures[ 0 ] ),
                            },
                        };
                        return descriptorSet;
                    }();

                    node->attachComponent< Reflective >();

                    return node;
                }()
          	);

            return Library::Scenes::Scene {
                .root = group,
            };
        }();

        m_library.passes.screen.uniforms = {
            [&] {
                return crimild::alloc< RenderPassUniformBuffer >();
            }()
        };

        m_library.passes.screen.descriptorSet = [&] {
            auto descriptorSet = crimild::alloc< DescriptorSet >();
            descriptorSet->descriptorSetLayout = m_library.programs.screen->descriptorSetLayouts[ 0 ];
            descriptorSet->descriptorPool = crimild::alloc< DescriptorPool >();
            descriptorSet->descriptorPool->descriptorSetLayout = descriptorSet->descriptorSetLayout;
            descriptorSet->writes = {
                {
                    .descriptorType = DescriptorType::UNIFORM_BUFFER,
                    .buffer = crimild::get_ptr( m_library.passes.screen.uniforms[ 0 ] ),
                },
            };
            return descriptorSet;
        }();

        m_library.passes.screen.color = [&] {
            auto att = crimild::alloc< Attachment >();
            att->usage = Attachment::Usage::COLOR_ATTACHMENT;
            att->format = Format::COLOR_SWAPCHAIN_OPTIMAL;
            return att;
        }();

#pragma mark - screen render pass
        m_library.passes.screen.renderPass = [&] {
            auto renderPass = crimild::alloc< RenderPass >();
            renderPass->attachments = { m_library.passes.screen.color };
            renderPass->commands = [&] {
                auto commandBuffer = crimild::alloc< CommandBuffer >();
                m_library.scenes.screen.root->perform(
                    Apply(
                        [&]( Node *node ) {
                            if ( auto renderState = node->getComponent< RenderStateComponent >() ) {
                                commandBuffer->bindGraphicsPipeline( crimild::get_ptr( renderState->pipeline ) );
                                commandBuffer->bindVertexBuffer( crimild::get_ptr( renderState->vbo ) );
                                commandBuffer->bindIndexBuffer( crimild::get_ptr( renderState->ibo ) );
                                commandBuffer->bindDescriptorSet( crimild::get_ptr( m_library.passes.screen.descriptorSet ) );
                                commandBuffer->bindDescriptorSet( crimild::get_ptr( renderState->descriptorSet ) );
                                commandBuffer->drawIndexed(
                                    renderState->ibo->getCount()
                                );
                            }
                        }
                    )
                );
                return commandBuffer;
            }();
            return renderPass;
        }();

		m_library.master = [&] {
            auto master = crimild::alloc< PresentationMaster >();
            master->colorAttachment = m_library.passes.screen.color;
			return master;
        }();

        if ( m_library.frameGraph->compile() ) {
            auto commands = m_library.frameGraph->recordCommands();
            setCommandBuffers( { commands } );
        }

        return true;
    }

    void update( void ) override
    {
        auto clock = Simulation::getInstance()->getSimulationClock();

		auto updateScene = [&]( auto &scene ) {
			scene->perform( UpdateComponents( clock ) );
			scene->perform( UpdateWorldState() );
		};

        updateScene( m_library.scenes.scene.root );
        updateScene( m_library.scenes.screen.root );

        GLFWVulkanSystem::update();
    }

    void stop( void ) override
    {
        if ( auto renderDevice = getRenderDevice() ) {
            renderDevice->waitIdle();
        }

        GLFWVulkanSystem::stop();
    }

private:
	Library m_library;
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

