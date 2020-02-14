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

        m_scene = [&] {
            auto scene = crimild::alloc< Group >();

            scene->attachNode( [] {
                auto path = FilePath {
                    .path = "assets/models/bunny.obj",
                };
                auto group = crimild::alloc< Group >();
                OBJLoader loader( path.getAbsolutePath() );
                if ( auto model = loader.load() ) {
                    group->attachNode( model );
                }
                group->attachComponent< RotationComponent >( Vector3f::UNIT_Y, 0.1f );
                return group;
            }());

            scene->attachNode([] {
                auto settings = Simulation::getInstance()->getSettings();
                auto width = settings->get< crimild::Real32 >( "video.width", 0 );
                auto height = settings->get< crimild::Real32 >( "video.height", 1 );
                auto camera = crimild::alloc< Camera >( 45.0f, width / height, 0.1f, 100.0f );
                camera->local().setTranslate( 0.0f, 5.0f, 5.0f );
                camera->local().lookAt( 0.75 * Vector3f::UNIT_Y );
                Camera::setMainCamera( camera );
                return camera;
            }());

            return scene;
        }();

        m_pipelines = std::vector< SharedPointer< Pipeline >> {
            [] {
                auto program = crimild::retain( ShaderProgramLibrary::getInstance()->get( constants::SHADER_PROGRAM_UNLIT_TEXTURE_P3N3TC2 ) );
                auto pipeline = crimild::alloc< Pipeline >();
                pipeline->program = program;
                pipeline->descriptorSetLayout = program->descriptorSetLayout;
                pipeline->attributeDescriptions = program->attributeDescriptions;
                pipeline->bindingDescription = program->bindingDescription;
                pipeline->viewport.dimensions = Rectf( 0.0f, 0.0f, 0.5f, 1.0f );
                return pipeline;
            }(),
            [] {
                auto program = crimild::retain( ShaderProgramLibrary::getInstance()->get( constants::SHADER_PROGRAM_UNLIT_TEXTURE_P3N3TC2 ) );
                auto pipeline = crimild::alloc< Pipeline >();
                pipeline->program = program;
                pipeline->descriptorSetLayout = program->descriptorSetLayout;
                pipeline->attributeDescriptions = program->attributeDescriptions;
                pipeline->bindingDescription = program->bindingDescription;
                pipeline->viewport.dimensions = Rectf( 0.5f, 0.0f, 0.5f, 1.0f );
                return pipeline;
            }(),
        };

        auto renderables = std::vector< RenderStateComponent * > { };
        m_scene->perform( Apply( [&]( Node *node ) {
            if ( auto renderState = node->getComponent< RenderStateComponent >() ) {
                renderState->prepare( crimild::get_ptr( m_pipelines[ 0 ]->descriptorSetLayout ) );
                renderables.push_back( renderState );
            }
        }));

        auto commandBuffer = [&] {
            auto commandBuffer = crimild::alloc< CommandBuffer >();

            commandBuffer->begin( CommandBuffer::Usage::SIMULTANEOUS_USE );
            commandBuffer->beginRenderPass( nullptr );

            auto viewport = ViewportDimensions { };
            viewport.dimensions.width() = 1.0f / crimild::Real32( m_pipelines.size() );

            for ( auto i = 0l; i < m_pipelines.size(); i++ ) {
                auto &p = m_pipelines[ i ];

                viewport.dimensions.x() = i * viewport.dimensions.width();

                commandBuffer->setViewport( viewport );

                for ( auto &r : renderables ) {
                    auto vbo = crimild::get_ptr( r->vbo );
                    auto ibo = crimild::get_ptr( r->ibo );
                    auto descriptors = crimild::get_ptr( r->descriptorSet );

                    commandBuffer->bindGraphicsPipeline( crimild::get_ptr( p ) );
                    commandBuffer->bindVertexBuffer( vbo );
                    commandBuffer->bindIndexBuffer( ibo );
                    commandBuffer->bindDescriptorSet( descriptors );

                    crimild::UInt32 indexCount = ibo->getSize() /  ibo->getStride();
                    commandBuffer->drawIndexed( indexCount );
                }
            }

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
    std::vector< SharedPointer< Pipeline >> m_pipelines;
};

int main( int argc, char **argv )
{
	crimild::init();
    crimild::vulkan::init();

    Log::setLevel( Log::Level::LOG_LEVEL_ALL );
	
    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "Pipelines", crimild::alloc< Settings >( argc, argv ) );
	sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );
	return sim->run();
}

