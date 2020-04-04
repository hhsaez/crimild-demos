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

namespace crimild {

    enum MemoryDomain {
        /**
         	\brief Owned by Device
         	No need to allocate memory on the Host since resources may be created
         	directly on the Device.
         	If resources are created on Host first, their data might be cleaned
         	after it has been uplaoded to Device.
         	In any case, if we need to access data for a resource from Host, it
         	must be transfered first from Device.
         */
        DEVICE,
        /**
         	\brief Owned by Host
         	Resources are created on Host and might be uploaded to a Device in
         	a later stage. If so, data will not be cleaned.
         	Data is already accessible from Host. Data changed on Host must be
         	uploaded to Device again as soon as possible.
         */
        HOST,
        /**
         	\brief Owned by Device. Cached on Host
         	Resources are created on Host and their data is trasfer to Device memory,
         	but we need a copy of the data still accessible on Host. Data changed on
         	Host is ignored by Device.
         */
        CACHED_HOST,
    };

}

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

        m_scene = [&] {
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
            	Image::Usage::COLOR_ATTACHMENT | Image::Usage::PRESENTATION
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

    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "Triangle", crimild::alloc< Settings >( argc, argv ) );
    sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );
    return sim->run();
}

