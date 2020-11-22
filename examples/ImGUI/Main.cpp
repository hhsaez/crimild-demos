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

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <Crimild.hpp>
#include <Crimild_GLFW.hpp>
#include <Crimild_Vulkan.hpp>

using namespace crimild;
using namespace crimild::glfw;

class ImGUIController {
public:
    struct VertexTransform {
        Vector2f translate;
        Vector2f scale;
    };

public:
    void start( void ) noexcept
    {
        IMGUI_CHECKVERSION();

        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        auto &io = ImGui::GetIO();
        //        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        //        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
        io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

        createFonts();
        createRenderObjects();

        // init for vulkan
        m_commandBuffer = crimild::alloc< CommandBuffer >();
    }

    void update( const Clock &c ) noexcept
    {
        auto &io = ImGui::GetIO();

        io.DeltaTime = Numericf::max( 1.0f / 60.0f, c.getDeltaTime() );

        prepareForRender();

        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        {
            //            ImGui::Begin( "Another Window" );
            //            ImGui::Text( "Hello from another window" );
            //            if ( ImGui::Button( "Close" ) ) {
            //                std::cout << "Should close window" << std::endl;
            //            }
            //            ImGui::End();
        }

        ImGui::Render();

        render( getCommandBuffer() );
    }

    void render( CommandBuffer *commandBuffer ) noexcept
    {
        commandBuffer->clear();
        commandBuffer->begin( CommandBuffer::Usage::SIMULTANEOUS_USE );
        commandBuffer->beginRenderPass( nullptr, nullptr );

        renderDrawData( commandBuffer );

        commandBuffer->endRenderPass( nullptr );
        commandBuffer->end();
    }

    void cleanup( void ) noexcept
    {
        ImGui::DestroyContext();
    }

    CommandBuffer *getCommandBuffer( void ) noexcept { return crimild::get_ptr( m_commandBuffer ); }

private:
    void createFonts( void ) noexcept
    {
        auto &io = ImGui::GetIO();

        io.Fonts->AddFontDefault();

        unsigned char *pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32( &pixels, &width, &height );

        auto image = crimild::alloc< Image >( width, height, 4, pixels, Image::PixelFormat::RGBA );
        m_fontAtlas = crimild::alloc< Texture >( image );

        int idx = 0;
        io.Fonts->TexID = ( ImTextureID )( intptr_t ) idx;
    }

    void createRenderObjects( void ) noexcept
    {
        m_pipeline = [ & ] {
            auto pipeline = crimild::alloc< GraphicsPipeline >();
            pipeline->setProgram(
                [] {
                    auto program = crimild::alloc< ShaderProgram >(
                        Array< SharedPointer< Shader > > {
                            Shader::withBinary(
                                Shader::Stage::VERTEX,
                                {
                                    .path = "assets/shaders/imgui.vert.spv",
                                } ),
                            Shader::withBinary(
                                Shader::Stage::FRAGMENT,
                                {
                                    .path = "assets/shaders/imgui.frag.spv",
                                } ),
                        } );
                    program->vertexLayouts = { VertexP2TC2C4::getLayout() };
                    program->descriptorSetLayouts = {
                        [] {
                            auto layout = crimild::alloc< DescriptorSetLayout >();
                            layout->bindings = {
                                {
                                    .descriptorType = DescriptorType::UNIFORM_BUFFER,
                                    .stage = Shader::Stage::VERTEX,
                                },
                                {
                                    .descriptorType = DescriptorType::TEXTURE,
                                    .stage = Shader::Stage::FRAGMENT,
                                },
                            };
                            return layout;
                        }()
                    };
                    return program;
                }() );
            //pipeline->depthState = DepthState::DISABLED;
            //pipeline->cullFaceState = CullFaceState::DISABLED;
            return pipeline;
        }();

        m_uniformBuffer = [ & ] {
            return crimild::alloc< UniformBuffer >( VertexTransform {} );
        }();

        m_descriptorSet = [ & ] {
            auto descriptorSet = crimild::alloc< DescriptorSet >();
            descriptorSet->descriptors = {
                Descriptor {
                    .descriptorType = DescriptorType::UNIFORM_BUFFER,
                    .obj = m_uniformBuffer,
                },
                Descriptor {
                    .descriptorType = DescriptorType::TEXTURE,
                    //                    .texture = crimild::get_ptr( Texture::CHECKERBOARD_32 ),
                    .obj = m_fontAtlas,
                }
            };
            return descriptorSet;
        }();
    }

    void prepareForRender( void ) noexcept
    {
        auto &io = ImGui::GetIO();
        if ( !io.Fonts->IsBuilt() ) {
            CRIMILD_LOG_ERROR( "Font atlas is not built!" );
            return;
        }

        auto width = Simulation::getInstance()->getSettings()->get< float >( "video.width", 0 );
        auto height = Simulation::getInstance()->getSettings()->get< float >( "video.height", 1 );
        io.DisplaySize = ImVec2( width, height );
        io.DisplayFramebufferScale = ImVec2( 2.0f, 2.0f );
    }

    void setupRenderState( ImDrawData *drawData )
    {
        auto scale = Vector2f(
            -2.0f / drawData->DisplaySize.x,
            2.0f / drawData->DisplaySize.y );
        auto translate = Vector2f(
            -1.0f - drawData->DisplayPos.x * scale.x(),
            -1.0f - drawData->DisplayPos.y * scale.y() );

        m_uniformBuffer->setValue(
            VertexTransform {
                .scale = scale,
                .translate = translate,
            } );
    }

    void renderDrawData( CommandBuffer *commandBuffer ) noexcept
    {
        auto drawData = ImGui::GetDrawData();

        setupRenderState( drawData );

        auto vertexCount = drawData->TotalVtxCount;
        auto indexCount = drawData->TotalIdxCount;
        if ( vertexCount == 0 || indexCount == 0 ) {
            return;
        }

        auto vbo = crimild::alloc< VertexP2TC2C4Buffer >( vertexCount );
        auto ibo = crimild::alloc< IndexUInt16Buffer >( indexCount );
        auto vertexDst = vbo->getData();
        auto indexDst = ibo->getData();
        for ( auto i = 0; i < drawData->CmdListsCount; i++ ) {
            const auto cmdList = drawData->CmdLists[ i ];
#if 1
            for ( auto j = 0l; j < cmdList->VtxBuffer.Size; j++ ) {
                auto vertex = cmdList->VtxBuffer[ j ];
                vertexDst[ j ] = {
                    .position = Vector2f( vertex.pos.x, vertex.pos.y ),
                    .texCoord = Vector2f( vertex.uv.x, vertex.uv.y ),
                    .color = RGBAColorf(
                        ( ( vertex.col >> 0 ) & 0xFF ) / 255.0f,
                        ( ( vertex.col >> 8 ) & 0xFF ) / 255.0f,
                        ( ( vertex.col >> 16 ) & 0xFF ) / 255.0f,
                        ( ( vertex.col >> 24 ) & 0xFF ) / 255.0f ),
                };
            }
            for ( auto j = 0l; j < cmdList->IdxBuffer.Size; j++ ) {
                indexDst[ j ] = cmdList->IdxBuffer[ j ];
            }
#else
            // Can't use it yet
            memcpy( vertexDst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof( ImDrawVert ) );
            memcpy( indexDst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof( ImDrawIdx ) );
#endif
            vertexDst += cmdList->VtxBuffer.Size;
            indexDst += cmdList->IdxBuffer.Size;
        }

        commandBuffer->bindGraphicsPipeline( crimild::get_ptr( m_pipeline ) );
        commandBuffer->bindVertexBuffer( crimild::get_ptr( vbo ) );
        commandBuffer->bindIndexBuffer( crimild::get_ptr( ibo ) );
        commandBuffer->bindDescriptorSet( crimild::get_ptr( m_descriptorSet ) );

        crimild::Size vertexOffset = 0;
        crimild::Size indexOffset = 0;
        for ( int i = 0; i < drawData->CmdListsCount; i++ ) {
            const auto cmds = drawData->CmdLists[ i ];
            for ( auto cmdIt = 0l; cmdIt < cmds->CmdBuffer.Size; cmdIt++ ) {
                const auto cmd = &cmds->CmdBuffer[ cmdIt ];
                if ( cmd->UserCallback != nullptr ) {
                    if ( cmd->UserCallback == ImDrawCallback_ResetRenderState ) {
                        // Do nothing?
                    } else {
                        cmd->UserCallback( cmds, cmd );
                    }
                } else {
                    commandBuffer->drawIndexed(
                        cmd->ElemCount,
                        cmd->IdxOffset + indexOffset,
                        cmd->VtxOffset + vertexOffset );
                }
            }
            indexOffset += cmds->IdxBuffer.Size;
            vertexOffset += cmds->VtxBuffer.Size;
        }
    }

private:
    SharedPointer< CommandBuffer > m_commandBuffer;
    SharedPointer< UniformBuffer > m_uniformBuffer;
    SharedPointer< Texture > m_fontAtlas;
    SharedPointer< GraphicsPipeline > m_pipeline;
    SharedPointer< DescriptorSet > m_descriptorSet;
};

class ExampleVulkanSystem : public GLFWVulkanSystem {
public:
    crimild::Bool start( void ) override
    {
        if ( !GLFWVulkanSystem::start() ) {
            return false;
        }

        m_imguiController.start();

        auto program = ShaderProgramLibrary::getInstance()->get( constants::SHADER_PROGRAM_UNLIT_P2C3_COLOR );

        auto pipeline = [ & ] {
            auto pipeline = crimild::alloc< Pipeline >();
            pipeline->program = crimild::retain( program );
            return pipeline;
        }();

        auto vbo = crimild::alloc< VertexP2C3Buffer >(
            containers::Array< VertexP2C3 > {
                {
                    .position = Vector2f( -0.5f, 0.5f ),
                    .color = RGBColorf( 1.0f, 0.0f, 0.0f ),
                },
                {
                    .position = Vector2f( -0.5f, -0.5f ),
                    .color = RGBColorf( 0.0f, 1.0f, 0.0f ),
                },
                {
                    .position = Vector2f( 0.5f, -0.5f ),
                    .color = RGBColorf( 0.0f, 0.0f, 1.0f ),
                },
                {
                    .position = Vector2f( 0.5f, 0.5f ),
                    .color = RGBColorf( 1.0f, 1.0f, 1.0f ),
                },
            } );

        auto ibo = crimild::alloc< IndexUInt32Buffer >(
            containers::Array< crimild::UInt32 > {
                0,
                1,
                2,
            } );

        auto texture = Texture::CHECKERBOARD;

        auto triBuilder = [ & ]( const Vector3f &position ) {
            auto node = crimild::alloc< Node >();

            auto renderable = node->attachComponent< RenderStateComponent >();
            renderable->pipeline = pipeline;
            renderable->vbo = vbo;
            renderable->ibo = ibo;
            renderable->uniforms = {
                [ & ] {
                    auto ubo = crimild::alloc< ModelViewProjectionUniformBuffer >();
                    ubo->node = crimild::get_ptr( node );
                    return ubo;
                }(),
            };
            renderable->textures = { texture };

            node->local().setTranslate( position );

            auto startAngle = Random::generate< crimild::Real32 >( 0, Numericf::TWO_PI );
            auto speed = Random::generate< crimild::Real32 >( -1.0f, 1.0f );

            node->attachComponent< LambdaComponent >(
                [ startAngle, speed ]( Node *node, const Clock &clock ) {
                    auto time = clock.getAccumTime();
                    node->local().rotate().fromAxisAngle( Vector3f::UNIT_Z, startAngle + ( speed * time * -90.0f * Numericf::DEG_TO_RAD ) );
                } );

            return node;
        };

        m_scene = [ & ] {
            auto scene = crimild::alloc< Group >();
            for ( auto x = -5.0f; x <= 5.0f; x += 1.0f ) {
                for ( auto z = -5.0f; z <= 5.0f; z += 1.0f ) {
                    scene->attachNode( triBuilder( Vector3f( x, 0.0f, z + ( 0.1f * x / 10.0f ) ) ) );
                }
            }
            scene->attachNode( [] {
                auto settings = Simulation::getInstance()->getSettings();
                auto width = settings->get< crimild::Real32 >( "video.width", 0 );
                auto height = settings->get< crimild::Real32 >( "video.height", 1 );
                auto camera = crimild::alloc< Camera >( 45.0f, width / height, 0.1f, 100.0f );
                camera->local().setTranslate( 5.0f, 10.0f, 10.0f );
                camera->local().lookAt( Vector3f::ZERO );
                Camera::setMainCamera( camera );
                return camera;
            }() );
            return scene;
        }();

        m_commandBuffer = [ this ] {
            auto commandBuffer = crimild::alloc< CommandBuffer >();

            commandBuffer->begin( CommandBuffer::Usage::SIMULTANEOUS_USE );
            commandBuffer->beginRenderPass( nullptr );

            m_scene->perform( Apply( [ commandBuffer ]( Node *node ) {
                if ( auto renderState = node->getComponent< RenderStateComponent >() ) {
                    renderState->commandRecorder( crimild::get_ptr( commandBuffer ) );
                }
            } ) );

            commandBuffer->endRenderPass( nullptr );
            commandBuffer->end();

            return commandBuffer;
        }();

        setCommandBuffers(
            {
                //            	m_commandBuffer,
                crimild::retain( m_imguiController.getCommandBuffer() ),
            } );

        return true;
    }

    void update( void ) override
    {
        auto clock = Simulation::getInstance()->getSimulationClock();
        m_scene->perform( UpdateComponents( clock ) );
        m_scene->perform( UpdateWorldState() );

        m_imguiController.update( clock );

        GLFWVulkanSystem::update();
    }

    void stop( void ) override
    {
        m_imguiController.cleanup();

        if ( auto renderDevice = getRenderDevice() ) {
            renderDevice->waitIdle();
        }

        m_scene = nullptr;

        GLFWVulkanSystem::stop();
    }

private:
    SharedPointer< Node > m_scene;
    SharedPointer< CommandBuffer > m_commandBuffer;
    ImGUIController m_imguiController;
};

int main( int argc, char **argv )
{
    crimild::init();
    crimild::vulkan::init();

    Log::setLevel( Log::Level::LOG_LEVEL_ALL );

    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "ImGUI", crimild::alloc< Settings >( argc, argv ) );
    sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );
    return sim->run();
}
