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

            scene->attachNode( [] {
                auto path = FilePath {
                    .path = "assets/models/sponza/sponza.obj",
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
                                            .path = "assets/shaders/depth.vert.spv"
                                        }.getAbsolutePath()
                                    )
                                ),
                                crimild::alloc< Shader >(
                                    Shader::Stage::FRAGMENT,
                                    FileSystem::getInstance().readFile(
                                        FilePath {
                                            .path = "assets/shaders/depth.frag.spv"
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
                            };
                            return layout;
                        }();
                        return program;
                    }();
//                    pipeline->polygonState = PolygonState::LINE;
                    return pipeline;
                }();
                if ( auto model = loader.load() ) {
                    group->attachNode( model );
                    model->perform( Apply( [&]( Node *node ) {
                        if ( auto rs = node->getComponent< RenderStateComponent >() ) {
                            rs->uniforms = {
                                [&] {
                                    auto uniform = crimild::alloc< ModelViewProjectionUniformBuffer >();
                                    uniform->node = crimild::get_ptr( model );
                                    return uniform;
                                }(),
                            };
                            rs->textures = { };
                        }
                    }));
                    model->local().setScale( 0.1f );
                }
                return group;
            }());

            scene->attachNode([] {
                auto camera = crimild::alloc< Camera >( 45.0f, 4.0f / 3.0f, 0.1f, 500.0f );
                camera->local().setTranslate( -103.914337f, 7.80385065f, -3.4776721f );
                camera->local().setRotate( Quaternion4f( 0.0996486619f, -0.701276481f, 0.10000769f, 0.698762059f ) );
                camera->attachComponent< FreeLookCameraComponent >();
                Camera::setMainCamera( camera );
                return camera;
            }());

            scene->perform( StartComponents() );

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

    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "Depth Buffer", crimild::alloc< Settings >( argc, argv ) );

    SharedPointer< ImageManager > imageManager = crimild::alloc< crimild::stb::ImageManager >();

    sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );

    return sim->run();
}

