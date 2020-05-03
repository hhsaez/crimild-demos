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

        auto program = ShaderProgramLibrary::getInstance()->get( constants::SHADER_PROGRAM_UNLIT_P2C3TC2_TEXTURE_COLOR );

        auto pipeline = [&] {
            auto pipeline = crimild::alloc< Pipeline >();
            pipeline->program = crimild::retain( program );
            return pipeline;
        }();

        auto vbo = crimild::alloc< VertexP2C3TC2Buffer >(
            containers::Array< VertexP2C3TC2 > {
                {
                    .position = Vector2f( -1.0f, 1.0f ),
                    .color = RGBColorf::ONE,
                    .texCoord = Vector2f( -1.0f, +2.0f ),
                },
                {
                    .position = Vector2f( -1.0f, -1.0f ),
                    .color = RGBColorf::ONE,
                    .texCoord = Vector2f( -1.0f, -1.0f ),
                },
                {
                    .position = Vector2f( 1.0f, -1.0f ),
                    .color = RGBColorf::ONE,
                    .texCoord = Vector2f( +2.0f, -1.0f ),
                },
                {
                    .position = Vector2f( 1.0f, 1.0f ),
                    .color = RGBColorf::ONE,
                    .texCoord = Vector2f( +2.0f, +2.0f ),
                },
            }
        );

        auto ibo = crimild::alloc< IndexUInt32Buffer >(
            containers::Array< crimild::UInt32 > {
                0, 1, 2,
                0, 2, 3,
            }
        );


        auto textureBuilder = []( Sampler::WrapMode wrapMode ) {
            auto texture = crimild::alloc< Texture >();
            texture->imageView = [&] {
                auto imageView = crimild::alloc< ImageView >();
                imageView->image = Image::CHECKERBOARD_4;
                return imageView;
            }();
            texture->sampler = [&] {
                auto sampler = crimild::alloc< Sampler >();
                sampler->setMinFilter( Sampler::Filter::NEAREST );
                sampler->setMagFilter( Sampler::Filter::NEAREST );
                sampler->setBorderColor( Sampler::BorderColor::INT_OPAQUE_WHITE );
                sampler->setWrapMode( wrapMode );
                sampler->setMaxLod( texture->imageView->image->getMipLevels() );
                return sampler;
            }();
            return texture;
        };

		auto quadBuilder = [&]( const Vector3f &position, Sampler::WrapMode wrapMode ) {
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
			renderable->textures = {
				textureBuilder( wrapMode ),
			};

			node->local().setTranslate( position );
			
			return node;
		};

        m_scene = [&] {
            auto scene = crimild::alloc< Group >();
            scene->attachNode( quadBuilder( Vector3f( -1.15f, +1.15f, 0.0 ), Sampler::WrapMode::REPEAT ) );
            scene->attachNode( quadBuilder( Vector3f( +1.15f, +1.15f, 0.0 ), Sampler::WrapMode::MIRRORED_REPEAT ) );
            scene->attachNode( quadBuilder( Vector3f( -1.15f, -1.15f, 0.0 ), Sampler::WrapMode::CLAMP_TO_EDGE ) );
            scene->attachNode( quadBuilder( Vector3f( +1.15f, -1.15f, 0.0 ), Sampler::WrapMode::CLAMP_TO_BORDER ) );
            scene->attachNode([] {
                auto camera = crimild::alloc< Camera >();
                camera->local().setTranslate( 0.0f, 0.0f, 6.0f );
                camera->local().lookAt( Vector3f::ZERO );
                Camera::setMainCamera( camera );
                return camera;
            }());
            return scene;
        }();

        m_frameGraph = [&] {
            auto graph = crimild::alloc< FrameGraph >();

            auto color = [&] {
                auto attachment = graph->create< Attachment >();
                attachment->usage = Attachment::Usage::COLOR_ATTACHMENT;
                attachment->format = Format::COLOR_SWAPCHAIN_OPTIMAL;
                return attachment;
            }();

            auto renderPass = graph->create< RenderPass >();
            renderPass->attachments = { color };
            renderPass->commands = [&] {
                auto commands = crimild::alloc< CommandBuffer >();
                m_scene->perform(
                    Apply(
                        [&]( Node *node ) {
                            if ( auto renderable = node->getComponent< RenderStateComponent > () ) {
                                renderable->commandRecorder(
                                    crimild::get_ptr( commands )
                                );
                            }
                        }
                    )
                );
                return commands;
            }();

            auto master = graph->create< PresentationMaster >();
            master->colorAttachment = color;

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
    SharedPointer< FrameGraph > m_frameGraph;
};

int main( int argc, char **argv )
{
    crimild::init();
    crimild::vulkan::init();

    Log::setLevel( Log::Level::LOG_LEVEL_ALL );

    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "Texture Wrapping", crimild::alloc< Settings >( argc, argv ) );

    SharedPointer< ImageManager > imageManager = crimild::alloc< crimild::stb::ImageManager >();

    sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );

    return sim->run();
}

