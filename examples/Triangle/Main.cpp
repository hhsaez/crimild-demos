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
//using namespace crimild::rendergraph;

/*
struct Vertex {
    Vector2f pos;
    RGBColorf color;

    static VkVertexInputBindingDescription getBindingDescription( void )
    {
        auto bindingDescription = VkVertexInputBindingDescription {
            .binding = 0,
            .stride = sizeof( Vertex ),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };
        return bindingDescription;
    }

    static std::vector< VkVertexInputAttributeDescription > getAttributeDescriptions( void )
    {
        return {
            VkVertexInputAttributeDescription {
                .binding = 0,
                .location = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof( Vertex, pos ),
            },
            VkVertexInputAttributeDescription {
                .binding = 0,
                .location = 1,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof( Vertex, color )
            }
        };
    }
};

struct UniformBufferObject {
    Matrix4f model;
    Matrix4f view;
    Matrix4f proj;
};
 */

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

    /*
    class RenderPass : public SharedObject, public RenderResourceImpl< RenderPass > {
    public:
        virtual ~RenderPass( void ) noexcept = default;

        CommandBuffer *getCommandBuffer( void ) noexcept
        {
            if ( m_commandBuffer != nullptr ) {
                return crimild::get_ptr( m_commandBuffer );
            }

            m_commandBuffer = crimild::alloc< CommandBuffer >();

            m_commandBuffer->begin();
            m_commandBuffer->beginRenderPass( this );

            recordCommands( crimild::get_ptr( m_commandBuffer ) );

            m_commandBuffer->endRenderPass( this );
            m_commandBuffer->end();

            return crimild::get_ptr( m_commandBuffer );
        }

        virtual void recordCommands( CommandBuffer *commandBuffer ) noexcept
        {
            auto scene = Simulation::getInstance()->getScene();
            scene->perform( Apply( [ commandBuffer ]( Node *node ) {
                if ( auto renderable = node->getComponent< Renderable >() ) {
                    renderable->recordCommands( commandBuffer );
                }
            }));
        }

    private:
        SharedPointer< CommandBuffer > m_commandBuffer;
    };
     */
//}

#if 0

class VulkanRenderable : public SharedObject {
public:
    struct Descriptor {
//        vulkan::RenderDevice *renderDevice;
//        vulkan::CommandPool *commandPool;
//        vulkan::RenderPass *renderPass;
//        vulkan::Swapchain *swapchain;
//        vulkan::Pipeline *pipeline;
//        vulkan::DescriptorSetLayout *descriptorSetLayout;
//        std::vector< vulkan::Framebuffer * > framebuffers;
//        std::vector< SharedPointer< vulkan::CommandBuffer >> commandBuffers;
//        Vector3f position;
    };

public:
    VulkanRenderable( Descriptor const &descriptor )
    {
        createVertexBuffer( descriptor );
        createIndexBuffer( descriptor );
        createDescriptorPool( descriptor );
        createUniformBuffers( descriptor );
        createDescriptorSets( descriptor );
        createCommandBuffers( descriptor );
    }

    virtual ~VulkanRenderable( void ) noexcept = default;

    CommandBuffer *getCommandBuffer( crimild::Size imageIndex ) noexcept
    {
        return crimild::get_ptr( m_commandBuffers[ imageIndex ] );
    }

private:
        crimild::Bool createVertexBuffer( Descriptor const &descriptor ) noexcept
        {
            m_vertexBuffer = crimild::alloc< VertexP2C3Buffer >(
                containers::Array< VertexP2C3 > {
                	{
                        .position = Vector2f( -0.5f, 0.5f ),
                        .color = RGBColorf( 1.0f, 0.0f, 0.0f ),
                    },
                	{
                        .position = Vector2f( -0.5f, -0.5f ),
                        .color = RGBColorf( 1.0f, 1.0f, 1.0f ),
                    },
                	{
                        .position = Vector2f( 0.5f, -0.5f ),
                        .color = RGBColorf( 0.0f, 0.0f, 1.0f ),
                	},
                    {
                        .position = Vector2f( 0.5f, 0.5f ),
                        .color = RGBColorf( 0.0f, 1.0f, 0.0f ),
                    },
            	}
            );

            return m_vertexBuffer != nullptr;
        }

        crimild::Bool createIndexBuffer( Descriptor const &descriptor ) noexcept
        {
            m_indexBuffer = crimild::alloc< IndexUInt32Buffer >(
            	containers::Array< crimild::UInt32 > {
					0, 1, 2,
                	0, 2, 3,
            	}
            );

            return m_indexBuffer != nullptr;
        }

        crimild::Bool createDescriptorPool( Descriptor const &descriptor ) noexcept
        {
            /*
            auto renderDevice = descriptor.renderDevice;
            auto swapchain = descriptor.swapchain;

            m_descriptorPool = renderDevice->create(
                vulkan::DescriptorPool::Descriptor {
                    .swapchain = swapchain
                }
            );
             */

            return m_descriptorPool != nullptr;
        }

        crimild::Bool createDescriptorSets( Descriptor const &descriptor ) noexcept
        {
            /*
            auto renderDevice = descriptor.renderDevice;
            auto swapchain = descriptor.swapchain;
            auto descriptorPool = crimild::get_ptr( m_descriptorPool );
            auto layout = descriptor.descriptorSetLayout;

            m_descriptorSets.resize( swapchain->images.size() );
            for ( crimild::Size i = 0; i < m_descriptorSets.size(); ++i ) {
                auto descriptorSet = renderDevice->create(
                    vulkan::DescriptorSet::Descriptor {
                        .descriptorPool = descriptorPool,
                        .layout = layout
                    }
                );
                descriptorSet->write(
                    crimild::get_ptr( m_uniformBuffers[ i ] ),
                    0,
                    sizeof( UniformBufferObject )
                );
                m_descriptorSets[ i ] = descriptorSet;
            }
             return m_descriptorSets.size() > 0;
             */

            return false;
        }

        crimild::Bool createCommandBuffers( Descriptor const &descriptor ) noexcept
        {
            /*
            auto pipeline = descriptor.pipeline;
            auto swapchain = descriptor.swapchain;
            auto imageCount = swapchain->imageViews.size();

            for ( auto i = 0l; i < imageCount; i++ ) {
                auto commandBuffer = descriptor.commandBuffers[ i ];
                commandBuffer->bindGraphicsPipeline(
                    pipeline
                );
                commandBuffer->bindVertexBuffer(
                    crimild::get_ptr( m_vertexBuffer )
                );
                commandBuffer->bindIndexBuffer(
                       crimild::get_ptr( m_indexBuffer )
                   );
                commandBuffer->bindDescriptorSets(
                    crimild::get_ptr( m_descriptorSets[ i ] ),
                    crimild::get_ptr( pipeline->layout )
                  );
                commandBuffer->drawIndexed( static_cast< crimild::UInt32 >( m_indexBuffer->size / sizeof( crimild::UInt32 ) ) );
            }
             */

            return true;
        }

        crimild::Bool createUniformBuffers( Descriptor const &descriptor ) noexcept
        {
            /*
            auto swapchain = descriptor.swapchain;

            m_uniformBuffers.resize( swapchain->images.size() );
            for ( crimild::Size i = 0; i < swapchain->images.size(); ++i ) {
                auto ubo = uniformBufferData( 0, swapchain->extent.width, swapchain->extent.height, descriptor.position );
                m_uniformBuffers[ i ] = crimild::alloc< UniformBufferImpl< UniformBufferObject >>( ubo );
//                updateUniformBuffer( i, descriptor );
            }
			*/
            return m_uniformBuffers.size() > 0;
        }

        UniformBufferObject uniformBufferData( crimild::Real32 time, crimild::Real32 width, crimild::Real32 height, const Vector3f &position ) noexcept
        {
            return UniformBufferObject {
                .model = [ position ]( crimild::Real32 time ) {
                    Transformation t;
                    t.setTranslate( position );
                    t.rotate().fromAxisAngle( Vector3f::UNIT_Z, time * -90.0f * Numericf::DEG_TO_RAD );
                    return t.computeModelMatrix();
                }( time ),
                .view = [] {
                    Transformation t;
                    t.setTranslate( 4.0f, 4.0f, 4.0f );
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
            };
        }

        void updateUniformBuffer( crimild::UInt32 currentImage, Descriptor const &descriptor ) noexcept
        {
            /*
            static const bool ENABLE_ROTATION = true;
            static auto startTime = std::chrono::high_resolution_clock::now();

            auto swapchain = descriptor.swapchain;

            auto currentTime = std::chrono::high_resolution_clock::now();
            auto time = ENABLE_ROTATION * std::chrono::duration< float, std::chrono::seconds::period >( currentTime - startTime ).count();

            auto ubo = uniformBufferData( time, swapchain->extent.width, swapchain->extent.height, descriptor.position );

//            m_uniformBuffers[ currentImage ]->getData()[ 0 ] = ubo;//update( &ubo );
             */
        }

private:
    SharedPointer< Buffer > m_vertexBuffer;
    SharedPointer< Buffer > m_indexBuffer;
    std::vector< SharedPointer< CommandBuffer >> m_commandBuffers;
    std::vector< SharedPointer< UniformBuffer >> m_uniformBuffers;
    SharedPointer< DescriptorPool > m_descriptorPool;
    std::vector< SharedPointer< DescriptorSet >> m_descriptorSets;
};

#endif

class ExampleVulkanSystem : public GLFWVulkanSystem {
public:
    crimild::Bool start( void ) override
    {
        /*
        if ( !GLFWVulkanSystem::start()
             || !createDescriptorSetLayout()
             || !createPipeline()
             || !createPreRenderCommandBuffers() ) {
            return false;
        }
         */
        if ( !GLFWVulkanSystem::start() ) {
            return false;
        }

        auto swapchain = getSwapchain();

//        auto renderPass = crimild::alloc< RenderPass >();

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

        m_scene = [ this, swapchain ] {
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
                        .color = RGBColorf( 1.0f, 1.0f, 1.0f ),
                    },
                    {
                        .position = Vector2f( 0.5f, -0.5f ),
                        .color = RGBColorf( 0.0f, 0.0f, 1.0f ),
                    },
                    {
                        .position = Vector2f( 0.5f, 0.5f ),
                        .color = RGBColorf( 0.0f, 1.0f, 0.0f ),
                    },
                }
            );
            renderable->ibo = crimild::alloc< IndexUInt32Buffer >(
                containers::Array< crimild::UInt32 > {
                    0, 1, 2,
                	0, 2, 3,
                }
            );
            renderable->ubo = [ swapchain ] () -> SharedPointer< UniformBuffer > {
                auto position = Vector3f::ZERO;
                auto width = swapchain->extent.width;
                auto height = swapchain->extent.height;
                auto time = 0;

                return crimild::alloc< ModelViewProjUniformBuffer >(
                    ModelViewProjUniform {
                        .model = []( crimild::Real32 time, const crimild::Vector3f &position ) {
                            Transformation t;
                            t.setTranslate( position );
                            t.rotate().fromAxisAngle( Vector3f::UNIT_Z, time * -90.0f * Numericf::DEG_TO_RAD );
                            return t.computeModelMatrix();
                        }( time, position ),
                        .view = [] {
                            Transformation t;
                            t.setTranslate( 4.0f, 4.0f, 4.0f );
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

            return node;
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

        /*
        auto renderDevice = getRenderDevice();
        auto commandPool = getCommandPool();
        auto swapchain = getSwapchain();
        auto renderPass = getRenderPass();
        auto pipeline = crimild::get_ptr( m_pipeline );
        auto descriptorSetLayout = crimild::get_ptr( m_descriptorSetLayout );
        auto imageCount = swapchain->images.size();

        std::vector< vulkan::Framebuffer * > framebuffers( imageCount );
        for ( int i = 0l; i < imageCount; i++ ) {
            framebuffers[ i ] = getFramebuffer( i );
        }

        m_renderables.push_back(
            crimild::alloc< VulkanRenderable >(
                 VulkanRenderable::Descriptor {
                    .renderDevice = renderDevice,
                    .commandPool = commandPool,
                    .swapchain = swapchain,
                    .renderPass = renderPass,
                    .pipeline = pipeline,
                    .descriptorSetLayout = descriptorSetLayout,
                    .framebuffers = framebuffers,
                    .commandBuffers = m_commandBuffers,
                    .position = Vector3f::UNIT_X,
                }
            )
        );

        m_renderables.push_back(
            crimild::alloc< VulkanRenderable >(
         		VulkanRenderable::Descriptor {
            		.renderDevice = renderDevice,
            		.commandPool = commandPool,
            		.swapchain = swapchain,
                    .renderPass = renderPass,
            		.pipeline = pipeline,
            		.descriptorSetLayout = descriptorSetLayout,
                    .framebuffers = framebuffers,
                    .commandBuffers = m_commandBuffers,
            		.position = -Vector3f::UNIT_X,
        		}
            )
        );

        createPostRenderCommandBuffers();


        setCommandBuffers( m_commandBuffers );
         */

        return true;
    }

    void update( void ) override
    {
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
    /*
    crimild::Bool createDescriptorSetLayout( void ) noexcept
    {
        m_descriptorSetLayout = getRenderDevice()->create(
            vulkan::DescriptorSetLayout::Descriptor {

            }
        );

        return m_descriptorSetLayout != nullptr;
    }

    crimild::Bool createPipeline( void ) noexcept
    {
        auto renderDevice = getRenderDevice();
        auto swapchain = getSwapchain();
        auto renderPass = getRenderPass();
        auto descriptorSetLayout = crimild::get_ptr( m_descriptorSetLayout );

        m_pipeline = renderDevice->create(
            vulkan::Pipeline::Descriptor {
                .program = crimild::alloc< ShaderProgram >(
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
                ),
                .renderPass = renderPass,
                .primitiveType = Primitive::Type::TRIANGLES,
                .viewport = Rectf( 0, 0, swapchain->extent.width, swapchain->extent.height ),
                .scissor = Rectf( 0, 0, swapchain->extent.width, swapchain->extent.height ),
                .bindingDescription = { Vertex::getBindingDescription() },
                .attributeDescriptions = Vertex::getAttributeDescriptions(),
                .setLayouts = {
                    descriptorSetLayout,
                },
            }
        );

        return m_pipeline != nullptr;
    }

    crimild::Bool createPreRenderCommandBuffers( void ) noexcept
    {
        auto renderDevice = getRenderDevice();
        auto commandPool = getCommandPool();
        auto renderPass = getRenderPass();
        auto swapchain = getSwapchain();
        auto imageCount = swapchain->imageViews.size();

        for ( auto i = 0l; i < imageCount; i++ ) {
            auto framebuffer = getFramebuffer( i );

            m_commandBuffers.push_back(
                [ framebuffer, renderDevice, commandPool, renderPass ]() {
                    auto commandBuffer = renderDevice->create(
                        vulkan::CommandBuffer::Descriptor {
                            .commandPool = commandPool,
                        }
                    );
                    commandBuffer->begin( vulkan::CommandBuffer::Usage::SIMULTANEOUS_USE );
                    commandBuffer->beginRenderPass(
                        renderPass,
                        framebuffer,
                        RGBAColorf( 0.0f, 0.0f, 0.0f, 1.0f )
                    );
                    return commandBuffer;
                }()
            );
        }

        return m_commandBuffers.size() > 0;
    }

    crimild::Bool createPostRenderCommandBuffers( void ) noexcept
    {
        auto swapchain = getSwapchain();
        auto imageCount = swapchain->imageViews.size();

        for ( auto i = 0l; i < imageCount; i++ ) {
            auto commandBuffer = m_commandBuffers[ i ];
            commandBuffer->endRenderPass();
            commandBuffer->end();
        }

        return m_commandBuffers.size() > 0;
    }
     */

private:
    SharedPointer< DescriptorSetLayout > m_descriptorSetLayout;
    SharedPointer< Pipeline > m_pipeline;
    SharedPointer< CommandBuffer > m_commandBuffer;
    SharedPointer< Node > m_scene;

//    std::vector< SharedPointer< VulkanRenderable >> m_renderables;
};

int main( int argc, char **argv )
{
	crimild::init();

    Log::setLevel( Log::Level::LOG_LEVEL_ALL );
	
    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "Triangle", crimild::alloc< Settings >( argc, argv ) );
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

