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

        m_offscreenScene = [&] {
            auto scene = crimild::alloc< Group >();

            scene->attachNode([&] {
                auto node = crimild::alloc< Node >();

                auto renderable = node->attachComponent< RenderStateComponent >();
                renderable->pipeline = [&] {
                    auto pipeline = crimild::alloc< Pipeline >();
                    pipeline->program = [&] {
                        return crimild::retain(
                            ShaderProgramLibrary::getInstance()->get(
                                constants::SHADER_PROGRAM_UNLIT_P2C3_COLOR
                            )
                        );
                    }();
                    pipeline->cullFaceState = CullFaceState::DISABLED;
                    return pipeline;
                }();
                renderable->vbo = crimild::alloc< VertexP2C3Buffer >(
                    containers::Array< VertexP2C3 > {
                        {
                            .position = Vector2f( 0.0f, 1.0f ),
                            .color = RGBColorf( 1.0f, 0.0f, 0.0f ),
                        },
                        {
                            .position = Vector2f( -1.0f, -1.0f ),
                            .color = RGBColorf( 0.0f, 1.0f, 0.0f ),
                        },
                        {
                            .position = Vector2f( 1.0f, -1.0f ),
                            .color = RGBColorf( 0.0f, 0.0f, 1.0f ),
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
                        return ubo;
                    }(),
                };
                renderable->textures = {};

                node->attachComponent< RotationComponent >( Vector3f::UNIT_Y, 0.1f );

                return node;
            }());

            scene->attachNode([] {
                auto camera = crimild::alloc< Camera >();
                camera->local().setTranslate( 0.0f, 0.0f, 3.0f );
                Camera::setMainCamera( camera );
                return camera;
            }());
            return scene;
        }();

        auto texture = [] {
            auto texture = crimild::alloc< Texture >();
            texture->imageView = [&] {
                auto imageView = crimild::alloc< ImageView >();
                imageView->image = crimild::alloc< Image >();
                return imageView;
            }();
            texture->sampler = [&] {
                auto sampler = crimild::alloc< Sampler >();
                sampler->setMinFilter( Sampler::Filter::NEAREST );
                sampler->setMagFilter( Sampler::Filter::NEAREST );
                return sampler;
            }();
            return texture;
        }();

        m_scene = [&] {
            auto program = ShaderProgramLibrary::getInstance()->get( constants::SHADER_PROGRAM_UNLIT_P2C3TC2_TEXTURE_COLOR );

            auto pipeline = [&] {
                auto pipeline = crimild::alloc< Pipeline >();
                pipeline->program = crimild::retain( program );
                pipeline->cullFaceState = CullFaceState::DISABLED;
                return pipeline;
            }();

            auto vbo = crimild::alloc< VertexP2C3TC2Buffer >(
                containers::Array< VertexP2C3TC2 > {
                    {
                        .position = Vector2f( -1.0f, 1.0f ),
                        .color = RGBColorf::ONE,
                        .texCoord = Vector2f( 0.0f, 1.0f ),
                    },
                    {
                        .position = Vector2f( -1.0f, -1.0f ),
                        .color = RGBColorf::ONE,
                        .texCoord = Vector2f( 0.0f, 0.0f ),
                    },
                    {
                        .position = Vector2f( 1.0f, -1.0f ),
                        .color = RGBColorf::ONE,
                        .texCoord = Vector2f( 1.0f, 0.0f ),
                    },
                    {
                        .position = Vector2f( 1.0f, 1.0f ),
                        .color = RGBColorf::ONE,
                        .texCoord = Vector2f( 1.0f, 1.0f ),
                    },
                }
            );

            auto ibo = crimild::alloc< IndexUInt32Buffer >(
                containers::Array< crimild::UInt32 > {
                    0, 1, 2,
                    0, 2, 3,
                }
            );

            auto scene = crimild::alloc< Group >();
            scene->attachNode(
                [&] {
                    auto node = crimild::alloc< Node >();

                    auto renderable = node->attachComponent< RenderStateComponent >();
                    renderable->pipeline = pipeline;
                    renderable->vbo = vbo;
                    renderable->ibo = ibo;
                    renderable->uniforms = {
                        [&] {
                            auto ubo = crimild::alloc< ModelViewProjectionUniformBuffer >();
                            ubo->node = crimild::get_ptr( node );
                            return ubo;
                        }(),
                    };
                    renderable->textures = { texture };

                    node->attachComponent< RotationComponent >( Vector3f::UNIT_Y, 0.05f );

                    return node;
                }()
            );
            scene->attachNode([] {
                auto camera = crimild::alloc< Camera >();
                camera->local().setTranslate( 0.0f, 0.0f, 3.0f );
                camera->local().lookAt( Vector3f::ZERO );
                Camera::setMainCamera( camera );
                return camera;
            }());
            return scene;
        }();

        m_frameGraph = [&] {
            auto graph = crimild::alloc< FrameGraph >();

            {
                // Offscreen

                auto color = [&] {
                    auto att = graph->create< Attachment >();
                    att->usage = Attachment::Usage::COLOR_ATTACHMENT;
                    att->format = Format::R8G8B8A8_UNORM;
                    att->imageView = texture->imageView;
                    return att;
                }();

                auto renderPass = graph->create< RenderPass >();
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
                renderPass->clearValue = {
                    .color = RGBAColorf( 1.0, 0.0, 0.0, 0.0f ),
                };
            }

            {
                auto color = [&] {
                    auto att = graph->create< Attachment >();
                    att->usage = Attachment::Usage::COLOR_ATTACHMENT;
                    att->format = Format::COLOR_SWAPCHAIN_OPTIMAL;
                    return att;
                }();

                auto renderPass = graph->create< RenderPass >();
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
                renderPass->clearValue = {
                    .color = RGBAColorf( 0.5f, 0.5f, 0.5f, 0.0f ),
                };

                auto master = graph->create< PresentationMaster >();
                master->colorAttachment = color;
            }

            return graph;
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
