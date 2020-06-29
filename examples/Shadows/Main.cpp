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

using namespace crimild;
using namespace crimild::glfw;

Matrix4f computeLightSpaceMatrix( Light *light ) noexcept
{
	if ( light == nullptr ) {
		return Matrix4f::IDENTITY;
	}

	auto proj = Frustumf( 60.0f, 1.0f, 0.1f, 100.0f ).computeProjectionMatrix();
	
	Transformation lightTransform;
	lightTransform.setRotate( light->getWorld().getRotate() );
	lightTransform.setTranslate( -30.0f * lightTransform.computeDirection() );
	auto view = lightTransform.computeModelMatrix().getInverse();

//	return proj * view;
    return view * proj; // WHY?!?!
}

class ShadowPass : public RenderPass {
private:
	class ShadowPassUniformBuffer : public UniformBuffer {
    private:
        struct Props {
            Matrix4f lightSpaceMatrix;
        };
        
	public:
        ShadowPassUniformBuffer( Light *light ) noexcept
            : UniformBuffer( Props { } ),
              light( light )
        {
            
        }
        
		virtual ~ShadowPassUniformBuffer( void ) = default;

		Light *light = nullptr;

		void onPreRender( void ) noexcept override
		{
			setValue(
				Props {
					.lightSpaceMatrix = computeLightSpaceMatrix( light ),
				}
			);
		}
	};
	
public:
	ShadowPass( Node *scene, Light *light ) noexcept
	{
		m_pipeline = [&] {
			auto pipeline = crimild::alloc< Pipeline >();
			pipeline->program = [&] {
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
							"assets/shaders/shadow.vert.spv"
						),
						createShader(
							Shader::Stage::FRAGMENT,
							"assets/shaders/shadow.frag.spv"
						),
						}
				);
				program->vertexLayouts = { VertexLayout::P3_N3_TC2 };
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
								.descriptorType = DescriptorType::UNIFORM_BUFFER,
								.stage = Shader::Stage::FRAGMENT,
							},
							{
								.descriptorType = DescriptorType::TEXTURE,
								.stage = Shader::Stage::FRAGMENT,
							},
						};
						return layout;
					}(),
				};
				return program;
			}();
			pipeline->viewport = { .scalingMode = ScalingMode::DYNAMIC };
			pipeline->scissor = { .scalingMode = ScalingMode::DYNAMIC };
            pipeline->depthState = [&] {
                auto depth = crimild::alloc< DepthState >( true );
                depth->setBiasEnabled( true );
                depth->setBiasConstantFactor( 1.5f );
                depth->setBiasSlopeFactor( 2.5f );
                return depth;
            }();
			return pipeline;
		}();
		
        m_color = [&] {
            auto att = crimild::alloc< Attachment >();
            // TODO: usage should be optional. It can be derived from frame graph, right?
            // TODO: implement automatic transition
            att->format = Format::R8G8B8A8_UNORM;
            att->imageView = crimild::alloc< ImageView >();
            att->imageView->image = crimild::alloc< Image >();
            return att;
        }();

        m_depth = [&] {
            auto att = crimild::alloc< Attachment >();
            att->format = Format::DEPTH_STENCIL_DEVICE_OPTIMAL;
			att->imageView = crimild::alloc< ImageView >();
			att->imageView->image = crimild::alloc< Image >();
            return att;
        }();

        m_uniforms = {
            [&] {
                return crimild::alloc< ShadowPassUniformBuffer >( light );
            }()
        };
		
        m_descriptorSet = [&] {
            auto descriptorSet = crimild::alloc< DescriptorSet >();
            descriptorSet->descriptors = {
                {
                    .descriptorType = DescriptorType::UNIFORM_BUFFER,
                    .obj = m_uniforms[ 0 ],
                },
            };
            return descriptorSet;
        }();

		attachments = {
            m_color,
			m_depth,
		};
		
		commands = [&] {
			auto commandBuffer = crimild::alloc< CommandBuffer >();
			auto viewport = ViewportDimensions { .scalingMode = ScalingMode::RELATIVE };
			commandBuffer->setViewport( viewport );
			commandBuffer->setScissor( viewport );
			scene->perform(
				Apply(
					[&]( Node *node ) {
						if ( auto renderState = node->getComponent< RenderStateComponent >() ) {
							commandBuffer->bindGraphicsPipeline( crimild::get_ptr( m_pipeline ) );
							commandBuffer->bindVertexBuffer( crimild::get_ptr( renderState->vbo ) );
							commandBuffer->bindIndexBuffer( crimild::get_ptr( renderState->ibo ) );
							commandBuffer->bindDescriptorSet( crimild::get_ptr( m_descriptorSet ) );
							commandBuffer->bindDescriptorSet( crimild::get_ptr( renderState->descriptorSet ) );
							commandBuffer->drawIndexed( renderState->ibo->getIndexCount() );
						}
					}
				)
			);
			return commandBuffer;
		}();

        extent = {
            .scalingMode = ScalingMode::FIXED,
            .width = 512.0f,
            .height = 512.0f,
        };
		
		clearValue = {
			.color = RGBAColorf( 1.0f, 0.0f, 0.0f, 1.0f ),
		};
	}

	virtual ~ShadowPass( void ) = default;

    inline Attachment *getColor( void ) noexcept { return crimild::get_ptr( m_color ); }

	inline Attachment *getDepth( void ) noexcept { return crimild::get_ptr( m_depth ); }

private:
	SharedPointer< Pipeline > m_pipeline;
    SharedPointer< Attachment > m_color;
	SharedPointer< Attachment > m_depth;
	containers::Array< SharedPointer< UniformBuffer >> m_uniforms;
	containers::Array< SharedPointer< Texture >> m_textures;
	SharedPointer< DescriptorSet > m_descriptorSet;
};

class ScenePass : public RenderPass {
private:
	class SceneUniforms : public UniformBuffer {
    private:
        struct Props {
            Matrix4f view;
            Matrix4f proj;
            Matrix4f lightSpace;
            Vector3f lightPos;
        };
        
	public:
        SceneUniforms( void ) noexcept
            : UniformBuffer( Props { } )              
        {
            
        }
        
		virtual ~SceneUniforms( void ) = default;

		Camera *camera = nullptr;
		Light *light = nullptr;

		void onPreRender( void ) noexcept override
		{
			setValue(
                Props {
                    .view = [&] {
                        if ( camera != nullptr ) {
                            // if a camera has been specified, we use that one to get the view matrix
                            return camera->getViewMatrix();
                        }
                        
                        if ( auto camera = Camera::getMainCamera() ) {
                            // if no camera has been set, let's use whatever's the main one
                            return camera->getViewMatrix();
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
                    .lightSpace = [&] {
                        return computeLightSpaceMatrix( light );
                    }(),
                    .lightPos = [&] {
                        return light != nullptr
					    ? light->getWorld().getTranslate()
					    : Vector3f( 100.0f, 100.0f, 100.0f );
                    }(),
                }
            );
		}
	};
	
public:
	ScenePass( Node *scene, Camera *camera, Light *light, Attachment *shadowMap ) noexcept
	{
		m_color = [&] {
            auto att = crimild::alloc< Attachment >();
			// TODO: usage should be optional. It can be derived from frame graph, right?
			// TODO: implement automatic transition
			att->format = Format::R8G8B8A8_UNORM;
			att->imageView = crimild::alloc< ImageView >();
			att->imageView->image = crimild::alloc< Image >();
            return att;
        }();

        m_depth = [&] {
            auto att = crimild::alloc< Attachment >();
            //att->usage = Attachment::Usage::DEPTH_STENCIL_ATTACHMENT;
            att->format = Format::DEPTH_STENCIL_DEVICE_OPTIMAL;
			att->imageView = crimild::alloc< ImageView >();
			att->imageView->image = crimild::alloc< Image >();
            return att;
        }();

        m_uniforms = {
            [&] {
                auto ubo = crimild::alloc< SceneUniforms >();
                ubo->camera = camera;
                ubo->light = light;
                return ubo;
            }()
        };

        m_textures = {
            [&] {
                auto texture = crimild::alloc< Texture >();
                texture->imageView = shadowMap->imageView;
                texture->sampler = [] {
                    auto sampler = crimild::alloc< Sampler >();
                    sampler->setMinFilter( Sampler::Filter::NEAREST );
                    sampler->setMagFilter( Sampler::Filter::NEAREST );
                    return sampler;
                }();
                return texture;
            }()
        };

        m_descriptorSet = [&] {
            auto descriptorSet = crimild::alloc< DescriptorSet >();
            descriptorSet->descriptors = {
                {
                    .descriptorType = DescriptorType::UNIFORM_BUFFER,
                    .obj = m_uniforms[ 0 ],
                },
                {
                    .descriptorType = DescriptorType::TEXTURE,
                    .obj = m_textures[ 0 ],
                },
            };
            return descriptorSet;
        }();

		attachments = {
			m_color,
			m_depth,
		};
		
		commands = [&] {
			auto commandBuffer = crimild::alloc< CommandBuffer >();
			auto viewport = ViewportDimensions { .scalingMode = ScalingMode::RELATIVE };
			commandBuffer->setViewport( viewport );
			commandBuffer->setScissor( viewport );
			scene->perform(
				Apply(
					[&]( Node *node ) {
						if ( auto renderState = node->getComponent< RenderStateComponent >() ) {
							commandBuffer->bindGraphicsPipeline( crimild::get_ptr( renderState->pipeline ) );
							commandBuffer->bindVertexBuffer( crimild::get_ptr( renderState->vbo ) );
							commandBuffer->bindIndexBuffer( crimild::get_ptr( renderState->ibo ) );
							commandBuffer->bindDescriptorSet( crimild::get_ptr( m_descriptorSet ) );
							commandBuffer->bindDescriptorSet( crimild::get_ptr( renderState->descriptorSet ) );
							commandBuffer->drawIndexed(
								renderState->ibo->getIndexCount()
							);
						}
					}
				)
			);
			return commandBuffer;
		}();

		clearValue = {
			.color = RGBAColorf( 59.0f / 255.0f, 53.0f / 255.0f, 97.0f / 255.0f, 1.0f ),
		};
	}

	virtual ~ScenePass( void ) = default;

	inline Attachment *getColor( void ) noexcept { return crimild::get_ptr( m_color ); }

	inline Attachment *getDepth( void ) noexcept { return crimild::get_ptr( m_depth ); }

private:
	SharedPointer< Attachment > m_color;
	SharedPointer< Attachment > m_depth;
	containers::Array< SharedPointer< UniformBuffer >> m_uniforms;
	containers::Array< SharedPointer< Texture >> m_textures;
	SharedPointer< DescriptorSet > m_descriptorSet;
};

class DebugPass : public RenderPass {
private:
	struct Uniforms {
		crimild::Int32 attachmentType;
	};
	
public:
	explicit DebugPass( containers::Array< Attachment * > const &inputs ) noexcept
	{
		m_pipeline = [&] {
			auto pipeline = crimild::alloc< Pipeline >();
			pipeline->program = [&] {
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
							"assets/shaders/debug.vert.spv"
						),
						createShader(
							Shader::Stage::FRAGMENT,
							"assets/shaders/debug.frag.spv"
						),
						}
				);
				program->descriptorSetLayouts = {
					[] {
						auto layout = crimild::alloc< DescriptorSetLayout >();
						layout->bindings = {
							{
								.descriptorType = DescriptorType::UNIFORM_BUFFER,
								.stage = Shader::Stage::FRAGMENT,
							},
							{
							 	.descriptorType = DescriptorType::TEXTURE,
							 	.stage = Shader::Stage::FRAGMENT,
							},
						};
						return layout;
					}(),
				};
				return program;
			}();
			pipeline->viewport = { .scalingMode = ScalingMode::DYNAMIC };
			pipeline->scissor = { .scalingMode = ScalingMode::DYNAMIC };
			return pipeline;
		}();
		
		m_color = [&] {
            auto att = crimild::alloc< Attachment >();
			// TODO: usage should be optional. It can be derived from frame graph, right?
            att->usage = Attachment::Usage::COLOR_ATTACHMENT; 
            att->format = Format::COLOR_SWAPCHAIN_OPTIMAL;
            return att;
        }();

        ViewportDimensions dimensions[] = {
    		{
        		.scalingMode = ScalingMode::SWAPCHAIN_RELATIVE,
                .dimensions = Rectf( 0.0f, 0.0f, 1.0f, 1.0f ),
            },
            {
                .scalingMode = ScalingMode::SWAPCHAIN_RELATIVE,
                .dimensions = Rectf( 0.8f, 0.8f, 0.175f, 0.175f ),
            },
            {
                .scalingMode = ScalingMode::SWAPCHAIN_RELATIVE,
                .dimensions = Rectf( 0.5f, 0.5f, 0.5f, 0.5f ),
            },
            {
                .scalingMode = ScalingMode::SWAPCHAIN_RELATIVE,
                .dimensions = Rectf( 0.5f, 0.0f, 0.5f, 0.5f ),
            },
        };

		m_inputs = inputs.map(
			[ &, index = 0 ]( auto input ) mutable -> Input {
            	auto texture = [&] {
                    auto texture = crimild::alloc< Texture >();
                    texture->imageView = input->imageView;
                    texture->sampler = [] {
                        auto sampler = crimild::alloc< Sampler >();
                        sampler->setMinFilter( Sampler::Filter::NEAREST );
                        sampler->setMagFilter( Sampler::Filter::NEAREST );
                        return sampler;
                    }();
                    return texture;
                }();
				auto uniforms = [&] {
					auto formatIsDepthStencil = [&] {
						auto format = input->format;
						switch ( format ) {
							case Format::DEPTH_16_UNORM:
							case Format::DEPTH_32_SFLOAT:
							case Format::DEPTH_16_UNORM_STENCIL_8_UINT:
							case Format::DEPTH_24_UNORM_STENCIL_8_UINT:
							case Format::DEPTH_32_SFLOAT_STENCIL_8_UINT:
							case Format::DEPTH_STENCIL_DEVICE_OPTIMAL:
								return true;
							default:
								return false;
						}
					}();
					return crimild::alloc< UniformBuffer >(
						Uniforms {
							.attachmentType = formatIsDepthStencil ? 1 : 0,
						}
					);
				}();
				auto descriptorSet = [&] {
					auto descriptorSet = crimild::alloc< DescriptorSet >();
					descriptorSet->descriptors = {
						{
							.descriptorType = DescriptorType::UNIFORM_BUFFER,
							.obj = uniforms,
						},
						{
							.descriptorType = DescriptorType::TEXTURE,
							.obj = texture,
						},
					};
					return descriptorSet;
				}();
				return Input {
					.uniforms = uniforms,
					.texture = texture,
					.descriptorSet = descriptorSet,
					.viewport = dimensions[ index++ ],
				};
			}
		);

		attachments = {
			m_color,
		};

		commands = [&] {
			auto commandBuffer = crimild::alloc< CommandBuffer >();
			m_inputs.each(
				[&]( auto &input ) {
					commandBuffer->setViewport( input.viewport );
					commandBuffer->setScissor( input.viewport );
                    commandBuffer->bindGraphicsPipeline( crimild::get_ptr( m_pipeline ) );
					commandBuffer->bindDescriptorSet( crimild::get_ptr( input.descriptorSet ) );
					commandBuffer->draw( 6 );
				}
			);
			return commandBuffer;
		}();

		clearValue = {
			.color = RGBAColorf( 0.0f, 1.0f, 0.0f, 0.0f ),
		};
	}

	virtual ~DebugPass( void ) noexcept = default;

	inline Attachment *getColor( void ) noexcept { return crimild::get_ptr( m_color ); }

private:
	SharedPointer< Attachment > m_color;
	SharedPointer< Pipeline > m_pipeline;

	struct Input {
		SharedPointer< UniformBuffer > uniforms;
		SharedPointer< Texture > texture;
		SharedPointer< DescriptorSet > descriptorSet;
		ViewportDimensions viewport;
	};
	containers::Array< Input > m_inputs;
};

struct RenderPassUniform {
    Matrix4f view;
    Matrix4f proj;
    Matrix4f lightSpace;
    Vector3f lightPos;
};

class RenderPassUniformBuffer : public UniformBuffer {
public:
    RenderPassUniformBuffer( void ) noexcept
        : UniformBuffer( RenderPassUniform { } )
    {
        
    }
    
	~RenderPassUniformBuffer( void ) = default;

	Camera *camera = nullptr;
	Light *light = nullptr;
	Matrix4f reflect = Matrix4f::IDENTITY;

	void onPreRender( void ) noexcept override
	{
		setValue(
            RenderPassUniform {
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

struct Library {
	SharedPointer< FrameGraph > frameGraph;
	SharedPointer< PresentationMaster > master;

	struct Scenes {
        struct Scene {
            SharedPointer< Group > root;
            SharedPointer< Camera > camera;
			SharedPointer< Light > light;
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
            SharedPointer< Attachment > depth;
            SharedPointer< DescriptorSet > descriptorSet;
            containers::Array< SharedPointer< UniformBuffer >> uniforms;
            containers::Array< SharedPointer< Texture >> textures;
		};

		Pass offscreen;
		Pass scene;
        Pass screen;
	} passes;
};

#pragma mark - ExampleVulkanSystem
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
            program->vertexLayouts = { VertexLayout::P3_N3_TC2 };
            program->descriptorSetLayouts = {
                [] {
                    auto layout = crimild::alloc< DescriptorSetLayout >();
                    layout->bindings = {
                        {
                            .descriptorType = DescriptorType::UNIFORM_BUFFER,
                            .stage = Shader::Stage::VERTEX,
                        },
                        {
                            .descriptorType = DescriptorType::TEXTURE,
                            .stage = Shader::Stage::FRAGMENT,
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
                            .descriptorType = DescriptorType::UNIFORM_BUFFER,
                            .stage = Shader::Stage::FRAGMENT,
                        },
                        {
                            .descriptorType = DescriptorType::TEXTURE,
                            .stage = Shader::Stage::FRAGMENT,
                        },
                    };
                    return layout;
                }(),
            };
            return program;
        }();

        m_library.scenes.scene.root = [&] {
            auto scene = crimild::alloc< Group >();

            scene->attachNode( [&] {
                auto path = FilePath {
                    .path = "assets/models/scene.obj",
                };
                auto group = crimild::alloc< Group >();
                OBJLoader loader( path.getAbsolutePath() );
                loader.pipeline = [&] {
                    auto pipeline = crimild::alloc< Pipeline >();
                    pipeline->program = m_library.programs.scene;
                    pipeline->viewport = { .scalingMode = ScalingMode::DYNAMIC };
                    pipeline->scissor = { .scalingMode = ScalingMode::DYNAMIC };
                    return pipeline;
                }();
                if ( auto model = loader.load() ) {
                    group->attachNode( model );
                }
                return group;
            }());

            scene->attachNode([] {
                auto camera = crimild::alloc< Camera >( 60.0f, 1.0f, 0.1f, 100.0f );
                camera->local().setTranslate( 15.0f, 20.0f, 20.0f );
                camera->local().lookAt( 1.0 * Vector3f::UNIT_Y );
                Camera::setMainCamera( camera );
                return camera;
            }());

            scene->attachNode(
            	[&] {
                	auto light = crimild::alloc< Light >( Light::Type::DIRECTIONAL );
                    light->local().setTranslate( 20.0f, 20.0f, 20.0f );
                    light->local().lookAt( Vector3f::ZERO );
                    m_library.scenes.scene.light = light;

                	auto pivot = crimild::alloc< Group >();
                	pivot->attachNode( light );
                	pivot->attachComponent< RotationComponent >( Vector3f::UNIT_Y, 0.05f );
                	return pivot;
            	}()
            );

            return scene;
        }();

		m_shadowPass = crimild::alloc< ShadowPass >(
			crimild::get_ptr( m_library.scenes.scene.root ),
			crimild::get_ptr( m_library.scenes.scene.light )
		);

		m_scenePass = crimild::alloc< ScenePass >(
			crimild::get_ptr( m_library.scenes.scene.root ),
			crimild::get_ptr( m_library.scenes.scene.camera ),
			crimild::get_ptr( m_library.scenes.scene.light ),
          	m_shadowPass->getDepth()
		);

		m_debugPass = crimild::alloc< DebugPass >(
			containers::Array< Attachment * > {
				m_scenePass->getColor(),
//            	m_scenePass->getDepth(),
//				m_shadowPass->getColor(),
                m_shadowPass->getDepth(),
			}
		);

        m_library.master = [&] {
            auto master = crimild::alloc< PresentationMaster >();
			master->colorAttachment = crimild::retain( m_debugPass->getColor() );
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

	SharedPointer< ShadowPass > m_shadowPass;
	SharedPointer< ScenePass > m_scenePass;
	SharedPointer< DebugPass > m_debugPass;
};

int main( int argc, char **argv )
{
    crimild::init();
    crimild::vulkan::init();

    Log::setLevel( Log::Level::LOG_LEVEL_ALL );

    auto settings = crimild::alloc< Settings >( argc, argv );
    settings->set( "video.width", 720 );
    settings->set( "video.height", 720 );

    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "Shadows", settings );

    SharedPointer< ImageManager > imageManager = crimild::alloc< crimild::stb::ImageManager >();

    sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );

    return sim->run();
}

