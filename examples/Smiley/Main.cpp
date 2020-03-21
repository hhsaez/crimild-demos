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

        auto pipeline = [&] {
            auto pipeline = crimild::alloc< Pipeline >();
            pipeline->program = [&] {
                auto program = crimild::alloc< ShaderProgram >(
                    containers::Array< SharedPointer< Shader >> {
                        crimild::alloc< Shader >(
                            Shader::Stage::VERTEX,
                            FileSystem::getInstance().readFile(
                                FilePath {
                                    .path = "assets/shaders/smiley.vert.spv",
                                }.getAbsolutePath()
                            )
                        ),
                        crimild::alloc< Shader >(
                            Shader::Stage::FRAGMENT,
                            FileSystem::getInstance().readFile(
                                FilePath {
                                    .path = "assets/shaders/smiley.frag.spv",
                                }.getAbsolutePath()
                            )
                        ),
                    }
                );
                program->attributeDescriptions = VertexP2TC2C4::getAttributeDescriptions( 0 );
                program->bindingDescription = VertexP2TC2C4::getBindingDescription( 0 );
                program->descriptorSetLayout = [] {
                    auto layout = crimild::alloc< DescriptorSetLayout >();
                    layout->bindings = {
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

        struct ContextDescriptor {
            Vector4f dimensions;
        };

        m_scene = [&] {
            auto node = crimild::alloc< Node >();
            node->attachComponent( [&] {
                auto renderable = crimild::alloc< RenderStateComponent >();
                renderable->pipeline = pipeline;
                renderable->vbo = [&] {
					return crimild::alloc< VertexP2TC2C4Buffer >(
                     	containers::Array< VertexP2TC2C4 > {
                        	{ .position = Vector2f( -1.0f, +1.0f ), .texCoord = Vector2f( 0.0f, 1.0f ) },
                            { .position = Vector2f( -1.0f, -1.0f ), .texCoord = Vector2f( 0.0f, 0.0f ) },
                            { .position = Vector2f( +1.0f, -1.0f ), .texCoord = Vector2f( 1.0f, 0.0f ) },
                            { .position = Vector2f( +1.0f, +1.0f ), .texCoord = Vector2f( 1.0f, 1.0f ) },
                    	}
                   	);
                }();
                renderable->ibo = [&] {
                    return crimild::alloc< IndexUInt32Buffer >(
                        containers::Array< IndexUInt32 > {
							0, 1, 2,
                        	0, 2, 3,
                    	}
                    );
                }();
                renderable->uniforms = {
                    [&] {
                        auto settings = Simulation::getInstance()->getSettings();
                        auto width = settings->get< float >( "video.width", 1024 );
                        auto height = settings->get< float >( "video.height", 768 );
                        return crimild::alloc< UniformBufferImpl< ContextDescriptor >>(
                       		ContextDescriptor {
								.dimensions = Vector4f( width, height, 0.0f, 0.0f ),
                        	}
                       	);
                    }(),
                };
                renderable->textures = { };
                return renderable;
            }());
            return node;
        }();

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

    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "Smiley", crimild::alloc< Settings >( argc, argv ) );

    SharedPointer< ImageManager > imageManager = crimild::alloc< crimild::stb::ImageManager >();

    sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );

    return sim->run();
}

