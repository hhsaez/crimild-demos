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
                    auto light = crimild::alloc< Light >( Light::Type::SPOT );
                    light->setAmbient( RGBAColorf( 0.1f, 0.0f, 0.0f, 0.0f ) );
                    light->setInnerCutoff( Numericf::DEG_TO_RAD * 25.0f );
                    light->setOuterCutoff( Numericf::DEG_TO_RAD * 50.0f );
                    light->setColor( RGBAColorf( 1.0f, 0.0f, 0.0f, 1.0f ) );
//                        light->setCastShadows( true );
                    light->local().setTranslate( -5.0f, 3.0f, 5.0f );
                    light->local().lookAt( Vector3f::ZERO );
                    lights.push_back( light );
                    return light;
                }()
            );

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

                	auto buildModel = [&]( const Vector3f &position, crimild::Real32 scale = 1.0f )
                    {
                        auto group = crimild::alloc< Group >();

                        auto path = FilePath {
                            .path = "assets/models/cube.obj",
                        };
                        OBJLoader loader( path.getAbsolutePath() );
                        loader.pipeline = pipeline;
                        if ( auto model = loader.load() ) {
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
                                                    struct LightData {
                                                        RGBAColorf ambient = RGBAColorf::ZERO;
                                                        RGBAColorf diffuse = RGBAColorf::ONE;
                                                        RGBAColorf specular = RGBAColorf::ONE;
                                                        Vector4f position = Vector4f::ZERO;
                                                        Vector4f direction = -Vector4f::UNIT_Z;
                                                        Vector4f attenuation = Vector4f::UNIT_X;
                                                        Vector4f cutoffs = Vector4f::ZERO;
                                                    };

                                                    auto light = lights[ 0 ];
                                                    light->perform( UpdateWorldState() );

                                                    return crimild::alloc< UniformBufferImpl< LightData > >(
                                                    	LightData {
                                                            .ambient = light->getAmbient().rgba(),
                                                        	.diffuse = light->getColor().rgba(),
                                                        	.specular = light->getColor().rgba(),
															.position = light->getWorld().getTranslate().xyzw(),
															.direction = light->getDirection().xyzw(),
                                                        	.attenuation = light->getAttenuation().xyzw(),
                                                        	.cutoffs = Vector4f(
                                                            	Numericf::cos( light->getInnerCutoff() ),
                                                                Numericf::cos( light->getOuterCutoff() ),
                                                                0.0f,
                                                            	0.0f
                                                            ),
                                                    	}
                                                  	);
                                                }(),
                                            };
                                        }
                                    }
                                )
                            );
                            group->attachNode( model );
                        }

                        group->local().setTranslate( position );
                        group->local().setScale( scale );

                        return group;
                    };

                	auto buildAnimatedModel = [&]( const Vector3f &position, crimild::Real32 size = 1.0f ) {
                        auto model = buildModel( position, size );
                        model->attachComponent< LambdaComponent >(
                            [
                            	position,
                                speed = Random::generate< crimild::Real32 >( 0.5f, 2.0f )
                            ]( Node *node, const Clock &c ) {
                                auto theta = speed * c.getAccumTime();
                                node->local().setTranslate( position + Numericf::sin( theta ) * Vector3f::UNIT_Y );
                            }
                        );
                        return model;
                    };

                    group->attachNode( buildModel( Vector3f( 0.0f, -52.0f, 0.0f ), 100.0f ) );
//                    group->attachNode( buildModel( Vector3f( -50.0f, 0.0f, 0.0f ), 50.0f ) );
//                    group->attachNode( buildModel( Vector3f( 0.0f, 0.0f, -50.0f ), 50.0f ) );

                	group->attachNode( buildAnimatedModel( Vector3f( -2.0f, 0.75f, -3.0f ), 1.25f ) );
                    group->attachNode( buildAnimatedModel( Vector3f( -2.5f, 0.5f, 0.0f ), 1.5f ) );
                    group->attachNode( buildAnimatedModel( Vector3f( -1.0f, 0.25f, 3.0f ) ) );

                    group->attachNode( buildAnimatedModel( Vector3f( 0.5f, 0.35f, -3.0f ) ) );
                    group->attachNode( buildAnimatedModel( Vector3f( 0.0f, 0.0f, 0.0f ) ) );
                    group->attachNode( buildAnimatedModel( Vector3f( 0.75f, 0.5f, 3.0f ) ) );

                    group->attachNode( buildAnimatedModel( Vector3f( 4.0f, 0.5f, -3.0f ) ) );
                    group->attachNode( buildAnimatedModel( Vector3f( 3.3f, 0.25f, 0.0f ) ) );
                    group->attachNode( buildAnimatedModel( Vector3f( 3.0f, 0.75f, 3.0f ) ) );


                	return group;
            	}()
          	);

            scene->attachNode([] {
                auto camera = crimild::alloc< Camera >();
                camera->local().setTranslate( 20.0f, 5.0f, 12.0f );
                camera->local().lookAt( Vector3f( 0.0f, 0.0f, -10.0f ) );
                Camera::setMainCamera( camera );
                return camera;
            }());

            return scene;
        }();

        m_scene->perform( StartComponents() );

        auto framebuffer = [&] {
            auto framebuffer = crimild::alloc< Framebuffer >();
            framebuffer->extent.scalingMode = ScalingMode::SWAPCHAIN_RELATIVE;
            framebuffer->attachments = {
                [&] {
                    auto imageView = crimild::alloc< ImageView >();
                    imageView->type = ImageView::Type::IMAGE_VIEW_SWAPCHAIN;
                    return imageView;
                }(),
                [&] {
                    auto imageView = crimild::alloc< ImageView >();
                    imageView->type = ImageView::Type::IMAGE_VIEW_2D;
                    imageView->image = [&] {
                        auto image = crimild::alloc< Image >();
                        image->extent.scalingMode = ScalingMode::SWAPCHAIN_RELATIVE;
                        image->format = Format::DEPTH_STENCIL_DEVICE_OPTIMAL;
                        image->usage = Image::Usage::DEPTH_STENCIL_ATTACHMENT;
                        return image;
                    }();
                    imageView->format = imageView->image->format;
                    return imageView;
                }(),
            };
            return framebuffer;
        }();

        auto renderPass = [&] {
            auto createAttachment = [&]( Format format, Image::Usage usage ) {
                auto att = crimild::alloc< Attachment >();
                att->format = format;
                att->usage = usage;
                return att;
            };

            auto colorAtt = createAttachment(
                Format::COLOR_SWAPCHAIN_OPTIMAL,
                Image::Usage::COLOR_ATTACHMENT
            );
            auto depthStencilAtt = createAttachment(
                Format::DEPTH_STENCIL_DEVICE_OPTIMAL,
                Image::Usage::DEPTH_STENCIL_ATTACHMENT
            );

            auto pass = crimild::alloc< RenderPass >();
            pass->attachments = {
                colorAtt,
                depthStencilAtt,
            };
            pass->subpasses = {
                [&] {
                    auto subpass = crimild::alloc< RenderSubpass >();
                    subpass->colorAttachments = { colorAtt };
                    subpass->depthStencilAttachment = depthStencilAtt;
                    subpass->commands = [&] {
                        auto commandBuffer = crimild::alloc< CommandBuffer >();
                        m_scene->perform( Apply( [ commandBuffer ]( Node *node ) {
                            if ( auto renderState = node->getComponent< RenderStateComponent >() ) {
                                renderState->commandRecorder( crimild::get_ptr( commandBuffer ) );
                            }
                        }));
                        return commandBuffer;
                    }();
                    return subpass;
                }(),
            };
            return pass;
        }();

        auto commandBuffer = [&] {
            auto commandBuffer = crimild::alloc< CommandBuffer >();
            commandBuffer->begin( CommandBuffer::Usage::SIMULTANEOUS_USE );
            commandBuffer->beginRenderPass( crimild::get_ptr( renderPass ), crimild::get_ptr( framebuffer ) );
            auto count = renderPass->subpasses.size();
            for ( auto i = 0l; i < count; i++ ) {
                if ( auto commands = crimild::get_ptr( renderPass->subpasses[ i ]->commands ) ) {
                    commandBuffer->bindCommandBuffer( commands );
                    if ( i < count - 1 ) {
//                        commandBuffer->nextSubpass();
                    }
                }
            }
            commandBuffer->endRenderPass( crimild::get_ptr( renderPass ) );
            commandBuffer->end();
            return commandBuffer;
        }();

        setCommandBuffers({ commandBuffer });

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

    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "Spotlight", crimild::alloc< Settings >( argc, argv ) );

    SharedPointer< ImageManager > imageManager = crimild::alloc< crimild::stb::ImageManager >();

    sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );
    return sim->run();
}

