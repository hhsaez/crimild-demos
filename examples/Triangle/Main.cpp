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

#include "Rendering/Buffer.hpp"
#include "Rendering/CommandBuffer.hpp"
#include "Rendering/DescriptorSet.hpp"
#include "Rendering/IndexBuffer.hpp"
#include "Rendering/Pipeline.hpp"
#include "Rendering/RenderPass.hpp"
#include "Rendering/UniformBuffer.hpp"
#include "Rendering/VertexBuffer.hpp"

using namespace crimild;
using namespace crimild::glfw;
using namespace crimild::vulkan;

namespace crimild {

    class Renderable : public NodeComponent {
        CRIMILD_IMPLEMENT_RTTI( crimild::Renderable )

    public:
        virtual ~Renderable( void ) noexcept = default;

        SharedPointer< Pipeline > pipeline;
        SharedPointer< Buffer > vbo;
        SharedPointer< Buffer > ibo;
        SharedPointer< UniformBuffer > ubo;
        SharedPointer< DescriptorSetLayout > descriptorSetLayout;
        SharedPointer< DescriptorPool > descriptorPool;
        SharedPointer< DescriptorSet > descriptorSet;
        SharedPointer< Primitive > primitive;

        virtual void recordCommands( CommandBuffer *commandBuffer ) noexcept
        {
            commandBuffer->bindGraphicsPipeline( crimild::get_ptr( pipeline ) );
//            commandBuffer->bindPrimitive( crimild::get_ptr( m_primitive ) );
            commandBuffer->bindVertexBuffer( crimild::get_ptr( vbo ) );
            commandBuffer->bindIndexBuffer( crimild::get_ptr( ibo ) );
            commandBuffer->bindUniformBuffer( crimild::get_ptr( ubo ) );
            commandBuffer->drawIndexed( primitive->getIndexBuffer()->getIndexCount() );
        }
    };

}

class ExampleVulkanSystem : public GLFWVulkanSystem {
public:
    crimild::Bool start( void ) override
    {
        if ( !GLFWVulkanSystem::start() ) {
            return false;
        }

        auto swapchain = getSwapchain();

        m_descriptorSetLayout = [] {
            auto descriptorSetLayout = crimild::alloc< DescriptorSetLayout >();
            descriptorSetLayout->descriptorType = DescriptorType::UNIFORM_BUFFER;
            descriptorSetLayout->descriptorCount = 1;
            descriptorSetLayout->stage = Shader::Stage::VERTEX;
            return descriptorSetLayout;
        }();

        m_pipeline = [ this, swapchain ] {
            auto pipeline = crimild::alloc< Pipeline >();
            pipeline->program = crimild::alloc< ShaderProgram >(
                containers::Array< SharedPointer< Shader >> {
                    crimild::alloc< Shader >(
                        Shader::Stage::VERTEX,
                        FileSystem::getInstance().readResourceFile( "assets/shaders/triangle.vert.spv" )
                    ),
                    crimild::alloc< Shader >(
                        Shader::Stage::FRAGMENT,
                        FileSystem::getInstance().readResourceFile( "assets/shaders/triangle.frag.spv" )
                    ),
                }
            );
            pipeline->primitiveType = Primitive::Type::TRIANGLES;
            pipeline->viewport = Rectf( 0, 0, swapchain->extent.width, swapchain->extent.height );
            pipeline->scissor = Rectf( 0, 0, swapchain->extent.width, swapchain->extent.height );
            pipeline->attributeDescriptions = VertexP2C3::getAttributeDescriptions( 0 );
            pipeline->bindingDescription = VertexP2C3::getBindingDescription( 0 );
//            pipeline->renderPass = crimild::get_ptr( renderPass );
            pipeline->descriptorSetLayout = m_descriptorSetLayout;
            return pipeline;
        }();

        auto renderableBuilder = [ this ]( const Vector3f &position ) {
            auto node = crimild::alloc< Node >();

            auto renderable = node->attachComponent< Renderable >();
            renderable->pipeline = m_pipeline;
			renderable->vbo = crimild::alloc< VertexP2C3Buffer >(
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
                }
            );
            renderable->ibo = crimild::alloc< IndexUInt32Buffer >(
                containers::Array< crimild::UInt32 > {
                    0, 1, 2,
                	//0, 2, 3,
                }
            );
            renderable->ubo = [] () -> SharedPointer< UniformBuffer > {
                return crimild::alloc< ModelViewProjUniformBuffer >();
            }();
            renderable->descriptorPool = [] {
                auto descriptorPool = crimild::alloc< DescriptorPool >();
                descriptorPool->descriptorType = DescriptorType::UNIFORM_BUFFER;
                return descriptorPool;
            }();
            renderable->descriptorSetLayout = m_pipeline->descriptorSetLayout;
            renderable->descriptorSet = [ renderable ] {
                auto descriptorSet = crimild::alloc< DescriptorSet >();
                descriptorSet->descriptorPool = renderable->descriptorPool;
                descriptorSet->descriptorSetLayout = renderable->descriptorSetLayout;
                descriptorSet->pipeline = renderable->pipeline;
                descriptorSet->buffer = renderable->ubo;
                return descriptorSet;
            }();

            auto startAngle = Random::generate< crimild::Real32 >( 0, Numericf::TWO_PI );
            auto speed = Random::generate< crimild::Real32 >( -1.0f, 1.0f );

            node->attachComponent< LambdaComponent >(
            	[ renderable, position, startAngle, speed ]( Node *node, const Clock &clock ) {
                    auto settings = Simulation::getInstance()->getSettings();
                    auto width = settings->get< crimild::Int32 >( "video.width", 0 );
                    auto height = settings->get< crimild::Int32 >( "video.height", 0 );
                	auto ubo = static_cast< ModelViewProjUniformBuffer * >( crimild::get_ptr( renderable->ubo ) );
                	auto time = clock.getAccumTime();

                    ubo->setData(
                     	ModelViewProjUniform {
                         	.model = []( crimild::Real32 time, const crimild::Vector3f &position, crimild::Real32 startAngle, crimild::Real32 speed ) {
                             	Transformation t;
                             	t.setTranslate( position );
                             	t.rotate().fromAxisAngle( Vector3f::UNIT_Z, startAngle + ( speed * time * -90.0f * Numericf::DEG_TO_RAD ) );
                             	return t.computeModelMatrix();
                         	}( time, position, startAngle, speed ),
                         	.view = [] {
                                Transformation t;
                             	t.setTranslate( 5.0f, 4.0f, 5.0f );
                             	t.lookAt( Vector3f::ZERO, Vector3f::UNIT_Y );
                             	return t.computeModelMatrix().getInverse();
                         	}(),
                         	.proj = []( float width, float height ) {
                            	auto frustum = Frustumf( 45.0f, width / height, 0.1f, 100.0f );
                             	auto proj = frustum.computeProjectionMatrix();

                             	// Invert Y-axis
                             	// This also needs to set front face as counter-clockwise for culling
                             	// when creating pipeline. In addition, we need to use a depth range
                             	// of [0, 1] for Vulkan
                             	// Check: https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
                             	static const auto CLIP_CORRECTION = Matrix4f(
                                 	1.0f, 0.0f, 0.0f, 0.0f,
                                 	0.0f, -1.0f, 0.0f, 0.0f,
                                 	0.0f, 0.0f, 0.5f, 0.5f,
                                 	0.0f, 0.0f, 0.0f, 1.0f
                             	).getTranspose(); // TODO: why do I need to transpose this matrix?

                             	return CLIP_CORRECTION * proj;
                         	}( width, height ),
                     	}
                 	);

                	auto pipeline = renderable->pipeline;
                    pipeline->viewport = Rectf( 0, 0, width, height );
                    pipeline->scissor = Rectf( 0, 0, width, height );
            	}
            );

            return node;
        };

        m_scene = [ renderableBuilder ] {
            auto group = crimild::alloc< Group >();
            for ( auto x = -5.0f; x <= 5.0f; x += 1.0f ) {
                for ( auto z = -5.0f; z <= 5.0f; z += 1.0f ) {
                    group->attachNode( renderableBuilder( Vector3f( x, 0.0f, z ) ) );
                }
            }
            return group;
        }();

        m_commandBuffer = [ this ] {
            auto commandBuffer = crimild::alloc< CommandBuffer >();

            commandBuffer->begin( CommandBuffer::Usage::SIMULTANEOUS_USE );
            commandBuffer->beginRenderPass( nullptr );

            m_scene->perform( Apply( [ commandBuffer ]( Node *node ) {
                if ( auto renderable = node->getComponent< Renderable >() ) {
                    commandBuffer->bindGraphicsPipeline(
                        crimild::get_ptr( renderable->pipeline )
                    );
                    commandBuffer->bindVertexBuffer(
                        crimild::get_ptr( renderable->vbo )
                    );
                    commandBuffer->bindIndexBuffer(
                       	crimild::get_ptr( renderable->ibo )
                    );
                    commandBuffer->bindDescriptorSet(
                        crimild::get_ptr( renderable->descriptorSet )
                  	);
                    crimild::UInt32 indexCount = renderable->ibo->getSize() /  renderable->ibo->getStride();
                    commandBuffer->drawIndexed(
                   		indexCount
                	);
                }
            }));

            commandBuffer->endRenderPass( nullptr );
            commandBuffer->end();

            return commandBuffer;
        }();

        auto commandBuffers = std::vector< SharedPointer< CommandBuffer >> { m_commandBuffer };
        setCommandBuffers( commandBuffers );

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
        m_pipeline = nullptr;
        m_commandBuffer = nullptr;
        m_descriptorSetLayout = nullptr;

        GLFWVulkanSystem::stop();
    }

private:
    SharedPointer< DescriptorSetLayout > m_descriptorSetLayout;
    SharedPointer< Pipeline > m_pipeline;
    SharedPointer< CommandBuffer > m_commandBuffer;
    SharedPointer< Node > m_scene;
};

int main( int argc, char **argv )
{
	crimild::init();

    Log::setLevel( Log::Level::LOG_LEVEL_ALL );
	
    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "Triangles", crimild::alloc< Settings >( argc, argv ) );
	sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );

//    auto scene = [] {
//
//        auto pipeline = crimild::alloc< Pipeline >();
//        pipeline->program = crimild::alloc< ShaderProgram >(
//            containers::Array< SharedPointer< Shader >> {
//                crimild::alloc< Shader >(
//                    Shader::Stage::VERTEX,
//                    FileSystem::getInstance().readResourceFile( "assets/shaders/triangle.vert.spv" )
//                ),
//                crimild::alloc< Shader >(
//                    Shader::Stage::FRAGMENT,
//                    FileSystem::getInstance().readResourceFile( "assets/shaders/triangle.frag.spv" )
//                ),
//            }
//        );
//
//
//        auto group = crimild::alloc< Group >();
//        return group;
//    }();

	/*
    auto scene = crimild::alloc< Group >();

    float vertices[] = {
        -1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f
    };

    unsigned short indices[] = {
        0, 1, 2
    };

    auto primitive = crimild::alloc< Primitive >();
    primitive->setVertexBuffer( crimild::alloc< VertexBufferObject >( VertexFormat::VF_P3_C4, 3, vertices ) );
    primitive->setIndexBuffer( crimild::alloc< IndexBufferObject >( 3, indices ) );

    auto geometry = crimild::alloc< Geometry >();
    geometry->attachPrimitive( primitive );
    auto material = crimild::alloc< Material >();
	material->setDiffuse( RGBAColorf( 0.0f, 1.0f, 0.0f, 1.0f ) );
	material->setProgram( crimild::alloc< VertexColorShaderProgram >() );
    material->getCullFaceState()->setEnabled( false );
    geometry->getComponent< MaterialComponent >()->attachMaterial( material );
    geometry->attachComponent< RotationComponent >( Vector3f( 0.0f, 1.0f, 0.0f ), 0.25f * Numericf::HALF_PI );
    scene->attachNode( geometry );

    auto camera = crimild::alloc< Camera >();
    camera->local().setTranslate( Vector3f( 0.0f, 0.0f, 3.0f ) );
    scene->attachNode( camera );
    
    sim->setScene( scene );
	*/
	return sim->run();
}

