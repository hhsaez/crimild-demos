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

struct RenderPassUniform {
    Matrix4f view;
    Matrix4f proj;
};

class RenderPassUniformBuffer : public UniformBufferImpl< RenderPassUniform > {
public:
    ~RenderPassUniformBuffer( void ) = default;

    Camera *camera = nullptr;

    void updateIfNeeded( void ) noexcept override
    {
        setData({
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
        };

        Scene scene;
    } scenes;

    struct Programs {
        SharedPointer< ShaderProgram > scene;
    } programs;

    struct Passes {
        struct Pass {
            SharedPointer< RenderPass > renderPass;
            SharedPointer< Attachment > color;
            SharedPointer< Attachment > depth;
            SharedPointer< DescriptorSet > descriptorSet;
            containers::Array< SharedPointer< UniformBuffer >> uniforms;
        };

        Pass scene;
    } passes;
};

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
            program->attributeDescriptions = VertexP3N3TC2::getAttributeDescriptions( 0 );
            program->bindingDescription = VertexP3N3TC2::getBindingDescription( 0 );
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
                            .descriptorType = DescriptorType::COMBINED_IMAGE_SAMPLER,
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
                    .path = "assets/models/bunny/bunny.obj",
                };
                auto group = crimild::alloc< Group >();
                OBJLoader loader( path.getAbsolutePath() );
                loader.pipeline = [&] {
                    auto pipeline = crimild::alloc< Pipeline >();
                    pipeline->program = m_library.programs.scene;
                    pipeline->descriptorSetLayouts = pipeline->program->descriptorSetLayouts;
                    pipeline->attributeDescriptions = pipeline->program->attributeDescriptions;
                    pipeline->bindingDescription = pipeline->program->bindingDescription;
                    pipeline->viewport = { .scalingMode = ScalingMode::DYNAMIC };
                    pipeline->scissor = { .scalingMode = ScalingMode::DYNAMIC };
                    return pipeline;
                }();
                if ( auto model = loader.load() ) {
                    group->attachNode( model );
                }
                group->attachComponent< RotationComponent >( Vector3f::UNIT_Y, 0.1f );
                return group;
            }());

            scene->attachNode([] {
                auto camera = crimild::alloc< Camera >();
                camera->local().setTranslate( 0.0f, 3.0f, 3.0f );
                camera->local().lookAt( 0.75 * Vector3f::UNIT_Y );
                Camera::setMainCamera( camera );
                return camera;
            }());

            return scene;
        }();

        m_library.passes.scene.uniforms = {
            [&] {
                auto ubo = crimild::alloc< RenderPassUniformBuffer >();
                ubo->camera = crimild::get_ptr( m_library.scenes.scene.camera );
                return ubo;
            }()
        };

        m_library.passes.scene.descriptorSet = [&] {
            auto descriptorSet = crimild::alloc< DescriptorSet >();
            descriptorSet->descriptorSetLayout = m_library.programs.scene->descriptorSetLayouts[ 0 ];
            descriptorSet->descriptorPool = crimild::alloc< DescriptorPool >();
            descriptorSet->descriptorPool->descriptorSetLayout = descriptorSet->descriptorSetLayout;
            descriptorSet->writes = {
                {
                    .descriptorType = DescriptorType::UNIFORM_BUFFER,
                    .buffer = crimild::get_ptr( m_library.passes.scene.uniforms[ 0 ] ),
                },
            };
            return descriptorSet;
        }();

        m_library.passes.scene.color = [&] {
            auto att = crimild::alloc< Attachment >();
            att->usage = Attachment::Usage::COLOR_ATTACHMENT;
            att->format = Format::COLOR_SWAPCHAIN_OPTIMAL;
            return att;
        }();

        m_library.passes.scene.depth = [&] {
            auto att = crimild::alloc< Attachment >();
            att->usage = Attachment::Usage::DEPTH_STENCIL_ATTACHMENT;
            att->format = Format::DEPTH_STENCIL_DEVICE_OPTIMAL;
            return att;
        }();

#pragma mark - scene renderpass
        m_library.passes.scene.renderPass = [&] {
            auto renderPass = crimild::alloc< RenderPass >();
            renderPass->attachments = {
                m_library.passes.scene.color,
                m_library.passes.scene.depth,
            };
            renderPass->commands = [&] {
                auto commandBuffer = crimild::alloc< CommandBuffer >();
                auto viewport = ViewportDimensions { .scalingMode = ScalingMode::RELATIVE };
                commandBuffer->setViewport( viewport );
                commandBuffer->setScissor( viewport );
                m_library.scenes.scene.root->perform(
                    Apply(
                        [&]( Node *node ) {
                            if ( auto renderState = node->getComponent< RenderStateComponent >() ) {
                                commandBuffer->bindGraphicsPipeline( crimild::get_ptr( renderState->pipeline ) );
                                commandBuffer->bindVertexBuffer( crimild::get_ptr( renderState->vbo ) );
                                commandBuffer->bindIndexBuffer( crimild::get_ptr( renderState->ibo ) );
                                commandBuffer->bindDescriptorSet( crimild::get_ptr( m_library.passes.scene.descriptorSet ) );
                                commandBuffer->bindDescriptorSet( crimild::get_ptr( renderState->descriptorSet ) );
                                commandBuffer->drawIndexed(
                                    renderState->ibo->getCount()
                                );
                            }
                        }
                    )
                );
                return commandBuffer;
            }();
            return renderPass;
        }();

        m_library.master = [&] {
            auto master = crimild::alloc< PresentationMaster >();
            master->colorAttachment = m_library.passes.scene.color;
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
};

int main( int argc, char **argv )
{
	crimild::init();
    crimild::vulkan::init();

    Log::setLevel( Log::Level::LOG_LEVEL_ALL );
	
    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "OBJ Loader", crimild::alloc< Settings >( argc, argv ) );

    SharedPointer< ImageManager > imageManager = crimild::alloc< crimild::stb::ImageManager >();

	sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );

    return sim->run();
}

