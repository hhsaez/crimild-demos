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

class ExampleVulkanSystem : public GLFWVulkanSystem {
public:
    crimild::Bool start( void ) override
    {
        if ( !GLFWVulkanSystem::start() ) {
            return false;
        }

        m_scene = [&] {
            auto scene = crimild::alloc< Group >();

            std::vector< SharedPointer< Light >> lights;

            scene->attachNode(
				[&] {
					auto group = crimild::alloc< Group >();

					auto pipeline = [&] {
						auto pipeline = crimild::alloc< Pipeline >();
						pipeline->program = [&] {
							auto program = crimild::alloc< ShaderProgram >(
								containers::Array< SharedPointer< Shader >> {
									crimild::alloc< Shader >(
										Shader::Stage::VERTEX,
										FileSystem::getInstance().readFile(
											FilePath {
												.path = "assets/shaders/unlit_color.vert.spv",
											}.getAbsolutePath()
										)
									),
									crimild::alloc< Shader >(
										Shader::Stage::FRAGMENT,
										FileSystem::getInstance().readFile(
											FilePath {
												.path = "assets/shaders/unlit_color.frag.spv",
											}.getAbsolutePath()
										)
									),
								}
							);
							program->attributeDescriptions = VertexP3N3TC2::getAttributeDescriptions( 0 );
							program->bindingDescription = VertexP3N3TC2::getBindingDescription( 0 );
							program->descriptorSetLayout = [] {
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
								};
								return layout;
							}();
							return program;
						}();
						return pipeline;
					}();

					auto light = [&]( const Quaternion4f &rotation, const RGBAColorf &color, float major, float minor, float speed ) {
						auto orbitingLight = crimild::alloc< Group >();

                        auto light = crimild::alloc< Light >();
                        light->setColor( color );
						orbitingLight->attachNode( light );
                        lights.push_back( light );
						
						auto path = FilePath {
							.path = "assets/models/sphere.obj",
						};
						OBJLoader loader( path.getAbsolutePath() );
						if ( auto model = loader.load() ) {
							model->local().setScale( 0.05f );
							model->perform(
								Apply(
									[&]( Node *node ) {
										if ( auto rs = node->getComponent< RenderStateComponent >() ) {
											rs->pipeline = pipeline;
											rs->uniforms = {
												[&] {
													auto ubo = crimild::alloc< ModelViewProjectionUniformBuffer >();
                                                    ubo->node = crimild::get_ptr( model );
													return ubo;
												}(),
												[&] {
                                                    return crimild::alloc< UniformBufferImpl< RGBAColorf >>( color );
												}(),
											};
											rs->textures = {};
										}
									}
								)
							);
							orbitingLight->attachNode( model );
						}
						
						orbitingLight->attachComponent< OrbitComponent >( 0.0f, 0.0f, major, minor, speed );
						
						auto group = crimild::alloc< Group >();
						group->attachNode( orbitingLight );
						group->local().setRotate( rotation );
						
						return group;
					};

					group->attachNode(
						light(
							Quaternion4f::createFromAxisAngle( Vector3f( 0.0f, 1.0f, 1.0 ).getNormalized(), Numericf::HALF_PI ),
							RGBAColorf( 1.0f, 0.0f, 0.0f, 1.0f ),
							-2.0f,
							2.25f,
							0.95f
						)
					);
					
					group->attachNode(
						light(
							Quaternion4f::createFromAxisAngle( Vector3f( 0.0f, 1.0f, -1.0 ).getNormalized(), -Numericf::HALF_PI ),
							RGBAColorf( 0.0f, 1.0f, 0.0f, 1.0f ),
							2.0f,
							2.25f,
							-0.75f
						)
					);
					
					group->attachNode(
						light(
							Quaternion4f::createFromAxisAngle( Vector3f( 1.0f, 1.0f, 0.0 ).getNormalized(), Numericf::HALF_PI ),
							RGBAColorf( 0.0f, 0.0f, 1.0f, 1.0f ),
							2.25f,
							2.0f,
							-0.85f
						)
					);
					
					return group;
				}()
			);

            scene->attachNode( [&] {
                auto path = FilePath {
                    .path = "assets/models/bunny/bunny.obj",
                };
                auto group = crimild::alloc< Group >();
                OBJLoader loader( path.getAbsolutePath() );
                loader.pipeline = [&] {
                    auto pipeline = crimild::alloc< Pipeline >();
                    pipeline->program = [&] {
                        auto program = crimild::alloc< ShaderProgram >(
                            containers::Array< SharedPointer< Shader >> {
                                crimild::alloc< Shader >(
                                    Shader::Stage::VERTEX,
                                    FileSystem::getInstance().readFile(
                                        FilePath {
                                            .path = "assets/shaders/phong.vert.spv",
                                        }.getAbsolutePath()
                                    )
                                ),
                                crimild::alloc< Shader >(
                                    Shader::Stage::FRAGMENT,
                                    FileSystem::getInstance().readFile(
                                        FilePath {
                                            .path = "assets/shaders/phong.frag.spv",
                                        }.getAbsolutePath()
                                    )
                                ),
                                }
                        );
                        program->attributeDescriptions = VertexP3N3TC2::getAttributeDescriptions( 0 );
                        program->bindingDescription = VertexP3N3TC2::getBindingDescription( 0 );
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
                                {
                                    .descriptorType = DescriptorType::UNIFORM_BUFFER,
                                    .stage = Shader::Stage::FRAGMENT,
                                },
                            };
                            return layout;
                        }();
                        return program;
                    }();
                    return pipeline;
                }();
                if ( auto model = loader.load() ) {
                    struct LightData {
                        RGBAColorf ambient = RGBAColorf::ZERO;
                        RGBAColorf diffuse = RGBAColorf::ONE;
                        RGBAColorf specular = RGBAColorf::ONE;
                        Vector4f position = Vector4f::ZERO;
                    };

                    struct Lights {
                        LightData lights[ 3 ];
                    };

                    class LightDataUniform : public UniformBufferImpl< Lights > {
                    public:
						void updateIfNeeded( void ) noexcept override
                        {
                            auto data = Lights { };
                            for ( int i = 0; i < 3; i++ ) {
                                auto &light = lights[ i ];
                                data.lights[ i ].diffuse = light->getColor();
                                data.lights[ i ].position = light->getWorld().getTranslate().xyzw();
                            }
                            setData( data );
                        }

                        std::vector< SharedPointer< Light >> lights;
                    };

                    model->local().setTranslate( 0.0f, -0.5f, 0.0f );
                    model->perform(
                        Apply(
                            [&]( Node *node ) {
                                if ( auto rs = node->getComponent< RenderStateComponent >() ) {
                                    rs->uniforms = {
                                        [&] {
                                            auto ubo = crimild::alloc< ModelViewProjectionUniformBuffer >();
                                            ubo->node = crimild::get_ptr( model );
                                            return ubo;
                                        }(),
                                        [&] {
                                            auto lightData = crimild::alloc< LightDataUniform >();
                                            lightData->lights = lights;
                                            return lightData;
                                        }(),
                                    };
                                }
                            }
                        )
                    );
                    group->attachNode( model );
                }
                return group;
            }());

            scene->attachNode([] {
                auto settings = Simulation::getInstance()->getSettings();
                auto width = settings->get< crimild::Real32 >( "video.width", 0 );
                auto height = settings->get< crimild::Real32 >( "video.height", 1 );
                auto camera = crimild::alloc< Camera >( 45.0f, width / height, 0.1f, 100.0f );
                camera->local().setTranslate( 0.0f, 0.5f, 8.0f );
                Camera::setMainCamera( camera );

                auto pivot = crimild::alloc< Group >();
                pivot->attachNode( camera );
                return pivot;
            }());

            return scene;
        }();

        m_scene->perform( StartComponents() );

        auto commandBuffer = [ this ] {
            auto commandBuffer = crimild::alloc< CommandBuffer >();

            commandBuffer->begin( CommandBuffer::Usage::SIMULTANEOUS_USE );
            commandBuffer->beginRenderPass( nullptr );

            m_scene->perform( Apply( [ commandBuffer ]( Node *node ) {
                if ( auto renderState = node->getComponent< RenderStateComponent >() ) {
                    renderState->commandRecorder( crimild::get_ptr( commandBuffer ) );
                }
            }));

            commandBuffer->endRenderPass( nullptr );
            commandBuffer->end();

            return commandBuffer;
        }();

        setCommandBuffers( { commandBuffer } );

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

        GLFWVulkanSystem::stop();
    }

private:
    SharedPointer< Node > m_scene;
};

int main( int argc, char **argv )
{
    crimild::init();
    crimild::vulkan::init();

    Log::setLevel( Log::Level::LOG_LEVEL_ALL );

    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "Lighting", crimild::alloc< Settings >( argc, argv ) );

    SharedPointer< ImageManager > imageManager = crimild::alloc< crimild::stb::ImageManager >();

    sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );
    return sim->run();
}

