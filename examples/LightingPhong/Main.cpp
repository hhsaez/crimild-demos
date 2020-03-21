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

        auto vbo = [&]( const Vector3f &size ) {
            auto halfWidth = 0.5f * size.x();
            auto halfHeight = 0.5f * size.y();
            auto halfDepth = 0.5f * size.z();
            return crimild::alloc< VertexP3N3TC2Buffer >(
                containers::Array< VertexP3N3TC2 > {
                    // top
                    { .position = Vector3f( -halfWidth, +halfHeight, -halfDepth ), .normal = Vector3f( 0.0f, 1.0f, 0.0f ), .texCoord = Vector2f( 0.0, 0.0 ) },
                    { .position = Vector3f( -halfWidth, +halfHeight, +halfDepth ), .normal = Vector3f( 0.0f, 1.0f, 0.0f ), .texCoord = Vector2f( 0.0, 0.0 ) },
                    { .position = Vector3f( +halfWidth, +halfHeight, +halfDepth ), .normal = Vector3f( 0.0f, 1.0f, 0.0f ), .texCoord = Vector2f( 0.0, 0.0 ) },
                    { .position = Vector3f( +halfWidth, +halfHeight, -halfDepth ), .normal = Vector3f( 0.0f, 1.0f, 0.0f ), .texCoord = Vector2f( 0.0, 0.0 ) },

                    // front
                    { .position = Vector3f( -halfWidth, +halfHeight, +halfDepth ), .normal = Vector3f( 0.0f, 0.0f, 1.0f ), .texCoord = Vector2f( 0.0, 0.0 ) },
                    { .position = Vector3f( -halfWidth, -halfHeight, +halfDepth ), .normal = Vector3f( 0.0f, 0.0f, 1.0f ), .texCoord = Vector2f( 0.0, 0.0 ) },
                    { .position = Vector3f( +halfWidth, -halfHeight, +halfDepth ), .normal = Vector3f( 0.0f, 0.0f, 1.0f ), .texCoord = Vector2f( 0.0, 0.0 ) },
                    { .position = Vector3f( +halfWidth, +halfHeight, +halfDepth ), .normal = Vector3f( 0.0f, 0.0f, 1.0f ), .texCoord = Vector2f( 0.0, 0.0 ) },

                    // back
                    { .position = Vector3f( +halfWidth, +halfHeight, -halfDepth ), .normal = Vector3f( 0.0f, 0.0f, -1.0f ), .texCoord = Vector2f( 0.0, 0.0 ) },
                    { .position = Vector3f( +halfWidth, -halfHeight, -halfDepth ), .normal = Vector3f( 0.0f, 0.0f, -1.0f ), .texCoord = Vector2f( 0.0, 0.0 ) },
                    { .position = Vector3f( -halfWidth, -halfHeight, -halfDepth ), .normal = Vector3f( 0.0f, 0.0f, -1.0f ), .texCoord = Vector2f( 0.0, 0.0 ) },
                    { .position = Vector3f( -halfWidth, +halfHeight, -halfDepth ), .normal = Vector3f( 0.0f, 0.0f, -1.0f ), .texCoord = Vector2f( 0.0, 0.0 ) },

                    // left
                    { .position = Vector3f( -halfWidth, +halfHeight, -halfDepth ), .normal = Vector3f( -1.0f, 0.0f, 0.0f ), .texCoord = Vector2f( 0.0, 0.0 ) },
                    { .position = Vector3f( -halfWidth, -halfHeight, -halfDepth ), .normal = Vector3f( -1.0f, 0.0f, 0.0f ), .texCoord = Vector2f( 0.0, 0.0 ) },
                    { .position = Vector3f( -halfWidth, -halfHeight, +halfDepth ), .normal = Vector3f( -1.0f, 0.0f, 0.0f ), .texCoord = Vector2f( 0.0, 0.0 ) },
                    { .position = Vector3f( -halfWidth, +halfHeight, +halfDepth ), .normal = Vector3f( -1.0f, 0.0f, 0.0f ), .texCoord = Vector2f( 0.0, 0.0 ) },

                    // right
                    { .position = Vector3f( +halfWidth, +halfHeight, +halfDepth ), .normal = Vector3f( 1.0f, 0.0f, 0.0f ), .texCoord = Vector2f( 0.0, 0.0 ) },
                    { .position = Vector3f( +halfWidth, -halfHeight, +halfDepth ), .normal = Vector3f( 1.0f, 0.0f, 0.0f ), .texCoord = Vector2f( 0.0, 0.0 ) },
                    { .position = Vector3f( +halfWidth, -halfHeight, -halfDepth ), .normal = Vector3f( 1.0f, 0.0f, 0.0f ), .texCoord = Vector2f( 0.0, 0.0 ) },
                    { .position = Vector3f( +halfWidth, +halfHeight, -halfDepth ), .normal = Vector3f( 1.0f, 0.0f, 0.0f ), .texCoord = Vector2f( 0.0, 0.0 ) },

                    // bottom
                    { .position = Vector3f( +halfWidth, -halfHeight, -halfDepth ), .normal = Vector3f( 0.0f, -1.0f, 0.0f ), .texCoord = Vector2f( 0.0, 0.0 ) },
                    { .position = Vector3f( +halfWidth, -halfHeight, +halfDepth ), .normal = Vector3f( 0.0f, -1.0f, 0.0f ), .texCoord = Vector2f( 0.0, 0.0 ) },
                    { .position = Vector3f( -halfWidth, -halfHeight, +halfDepth ), .normal = Vector3f( 0.0f, -1.0f, 0.0f ), .texCoord = Vector2f( 0.0, 0.0 ) },
                    { .position = Vector3f( -halfWidth, -halfHeight, -halfDepth ), .normal = Vector3f( 0.0f, -1.0f, 0.0f ), .texCoord = Vector2f( 0.0, 0.0 ) },
                }
            );
        };

        auto ibo = crimild::alloc< IndexUInt32Buffer >(
            containers::Array< crimild::UInt32 > {
                0, 1, 2, 0, 2, 3,
                4, 5, 6, 4, 6, 7,
                8, 9, 10, 8, 10, 11,
                12, 13, 14, 12, 14, 15,
                16, 17, 18, 16, 18, 19,
                20, 21, 22, 20, 22, 23
            }
        );

        struct LightDescriptor {
            RGBAColorf color;
            Vector4f position;
        };

        auto lightUBO = crimild::alloc< UniformBufferImpl< LightDescriptor >>(
            LightDescriptor {
            	.color = RGBAColorf( 1.0f, 1.0f, 1.0f, 1.0f ),
                .position = Vector4f( 10.0f, 5.0f, 10.0f, 1.0f ),
        	}
       	);

        auto box = [&]( const Vector3f &position, float scale, const RGBAColorf &color ) {
            auto node = crimild::alloc< Group >();
            node->attachComponent( [&] {
                auto renderable = crimild::alloc< RenderStateComponent >();
            	renderable->pipeline = pipeline;
            	renderable->vbo = vbo( Vector3f::ONE );
            	renderable->ibo = ibo;
            	renderable->uniforms = {
                	[&] {
                    	auto ubo = crimild::alloc< ModelViewProjectionUniformBuffer >();
                    	ubo->node = crimild::get_ptr( node );
                    	return ubo;
                	}(),
                    [&] {
                        return crimild::alloc< UniformBufferImpl< RGBAColorf >>( color );
                    }(),
                    lightUBO,
            	};
                renderable->textures = { };
                return renderable;
            }());
            node->local().setTranslate( position );
            node->local().setScale( scale );
            node->attachComponent< LambdaComponent >(
                [
                    position,
                    speed = Random::generate< crimild::Real32 >( 0.5f, 2.0f )
                ]( Node *node, const Clock &c ) {
                    auto theta = speed * c.getAccumTime();
                    node->local().setTranslate( position + Numericf::sin( theta ) * Vector3f::UNIT_Y );
                }
            );
            return node;
        };

        auto floor = [&]( const RGBAColorf &color ) {
            auto node = crimild::alloc< Group >();
            node->attachComponent( [&] {
                auto renderable = crimild::alloc< RenderStateComponent >();
                renderable->pipeline = pipeline;
                renderable->vbo = vbo( Vector3f::ONE );
                renderable->ibo = ibo;
                renderable->uniforms = {
                    [&] {
                        auto ubo = crimild::alloc< ModelViewProjectionUniformBuffer >();
                        ubo->node = crimild::get_ptr( node );
                        return ubo;
                    }(),
                    [&] {
                        return crimild::alloc< UniformBufferImpl< RGBAColorf >>( color );
                    }(),
                    lightUBO,
                };
                renderable->textures = { };
                return renderable;
            }());
            node->local().setTranslate( -20.0f * Vector3f::UNIT_Y );
            node->local().setScale( 30.0f );
            return node;
        };

        m_scene = [&] {
            auto scene = crimild::alloc< Group >();
            scene->attachNode( floor( RGBAColorf::ONE ) );
            scene->attachNode( box( Vector3f( 0.0f, 0.0f, -2.0f ), 1.0, RGBAColorf( 1.0f, 0.0f, 0.0f, 1.0f ) ) );
            scene->attachNode( box( Vector3f( 1.0f, 0.0f, 1.0f ), 2.0, RGBAColorf( 1.0f, 1.0f, 0.0f, 1.0f ) ) );
            scene->attachNode( box( Vector3f( -4.0f, 0.0f, -3.0f ), 1.5, RGBAColorf( 0.0f, 1.0f, 0.0f, 1.0f ) ) );
            scene->attachNode( box( Vector3f( 3.0f, 0.0f, -1.0f ), 0.5, RGBAColorf( 0.0f, 1.0f, 1.0f, 1.0f ) ) );
            scene->attachNode( box( Vector3f( -2.0f, 1.0f, 0.5f ), 0.5, RGBAColorf( 0.0f, 0.0f, 1.0f, 1.0f ) ) );
            scene->attachNode( box( Vector3f( -3.0f, 0.0f, 2.0f ), 1.0, RGBAColorf( 1.0f, 0.0f, 1.0f, 1.0f ) ) );
            scene->attachNode([] {
                auto camera = crimild::alloc< Camera >();
                camera->local().setTranslate( 5.0f, 5.0f, 5.0f );
                camera->local().lookAt( Vector3f::ZERO );
                Camera::setMainCamera( camera );
                return camera;
            }());
            return scene;
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

    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "Lighting: Blinn-Phong", crimild::alloc< Settings >( argc, argv ) );

    SharedPointer< ImageManager > imageManager = crimild::alloc< crimild::stb::ImageManager >();

    sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );

    return sim->run();
}

