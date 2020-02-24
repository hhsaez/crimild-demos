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

		auto createShaderProgram = []( std::string programName ) {
			auto program = crimild::alloc< ShaderProgram >(
				containers::Array< SharedPointer< Shader >> {
					crimild::alloc< Shader >(
						Shader::Stage::VERTEX,
						FileSystem::getInstance().readFile(
							FilePath {
								.path = "assets/shaders/" + programName + ".vert.spv"
							}.getAbsolutePath()
						)
					),
					crimild::alloc< Shader >(
						Shader::Stage::FRAGMENT,
						FileSystem::getInstance().readFile(
							FilePath {
								.path = "assets/shaders/" + programName + ".frag.spv"
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
                        .descriptorCount = 1,
                        .stage = Shader::Stage::VERTEX,
                    },
                    {
                        .descriptorType = DescriptorType::COMBINED_IMAGE_SAMPLER,
                        .descriptorCount = 1,
                        .stage = Shader::Stage::FRAGMENT,
                    }
                };
                return layout;
            }();
			return program;
		};

        m_scene = [&] {
            auto scene = crimild::alloc< Group >();

            // Skybox must go first for the moment
            auto skybox = [] {
                return crimild::alloc< Skybox >(
                    containers::Array< SharedPointer< Image >> {
                        crimild::ImageManager::getInstance()->loadImage( { .filePath = "assets/textures/right.png" } ),
                        crimild::ImageManager::getInstance()->loadImage( { .filePath = "assets/textures/left.png" } ),
                        crimild::ImageManager::getInstance()->loadImage( { .filePath = "assets/textures/top.png" } ),
                        crimild::ImageManager::getInstance()->loadImage( { .filePath = "assets/textures/bottom.png" } ),
                        crimild::ImageManager::getInstance()->loadImage( { .filePath = "assets/textures/back.png" } ),
                        crimild::ImageManager::getInstance()->loadImage( { .filePath = "assets/textures/front.png" } ),
                    }
                );
            }();
            scene->attachNode( skybox );

            auto loadModel = []( SharedPointer< Pipeline > const &pipeline, SharedPointer< Texture > const &texture, const Vector3f &position ) {
                auto path = FilePath {
                    .path = "assets/models/sphere.obj",
                };
                auto group = crimild::alloc< Group >();
                OBJLoader loader( path.getAbsolutePath() );
                if ( auto model = loader.load() ) {
                    model->perform( Apply( [&]( Node *node ) {
                        if ( auto rs = node->getComponent< RenderStateComponent >() ) {
                            rs->pipeline = pipeline;
                            rs->uniforms = {
                                [&] {
                                    auto ubo = crimild::alloc< ModelViewProjectionUniformBuffer >();
                                    ubo->node = node;
                                    return ubo;
                                }(),
                            };
                            rs->textures = { texture };
                        }
                    }));
                    group->attachNode( model );
                }
                group->attachComponent< RotationComponent >( Vector3f::UNIT_Y, 0.1f );
                group->local().setTranslate( position );
                return group;
            };

            auto pivot = crimild::alloc< Group >();

            pivot->attachNode( [&] {
                auto pipeline = [&] {
                    auto pipeline = crimild::alloc< Pipeline >();
					pipeline->program = createShaderProgram( "reflection" );
                    return pipeline;
                }();
                auto texture = crimild::retain( skybox->getTexture() );
                return loadModel( pipeline, texture, -1.125f * Vector3f::UNIT_X );
            }());

            pivot->attachNode( [&] {
                auto pipeline = [&] {
                    auto pipeline = crimild::alloc< Pipeline >();
					pipeline->program = createShaderProgram( "refraction" );
                    return pipeline;
                }();
                auto texture = crimild::retain( skybox->getTexture() );
                return loadModel( pipeline, texture, 1.125f * Vector3f::UNIT_X );
            }());

            pivot->attachNode([] {
                auto settings = Simulation::getInstance()->getSettings();
                auto width = settings->get< crimild::Real32 >( "video.width", 0 );
                auto height = settings->get< crimild::Real32 >( "video.height", 1 );
                auto camera = crimild::alloc< Camera >( 45.0f, width / height, 0.1f, 100.0f );
                camera->local().setTranslate( 0.0f, 0.0f, 10.0f );
                camera->local().lookAt( Vector3f::ZERO );
                Camera::setMainCamera( camera );

                auto pivot = crimild::alloc< Group >();
                pivot->attachNode( camera );
                return pivot;
            }());

            pivot->attachComponent< RotationComponent >( Vector3f::UNIT_Y, .0125f );
            scene->attachNode( pivot );

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

    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "Environment Map: Reflection & Refraction", crimild::alloc< Settings >( argc, argv ) );

    SharedPointer< ImageManager > imageManager = crimild::alloc< crimild::stb::ImageManager >();

    sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );
    return sim->run();
}

