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

namespace crimild {

    template< typename T >
    class DynamicUniformBuffer : public UniformBuffer {
    private:
        using PreRenderCallback = std::function< T( void ) >;

    public:
        explicit DynamicUniformBuffer( const T &value ) noexcept : UniformBuffer( value ) { }
        explicit DynamicUniformBuffer( PreRenderCallback callback ) noexcept : UniformBuffer( T() ) { onPreRender( callback ); }
        virtual ~DynamicUniformBuffer( void ) = default;

        inline void onPreRender( PreRenderCallback callback ) noexcept { m_callback = callback; }

        virtual void onPreRender( void ) noexcept override
        {
            if ( m_callback != nullptr ) {
                setValue( m_callback() );
            }
        }

    private:
        std::function< T( void ) > m_callback;
    };

}

using namespace crimild;
using namespace crimild::glfw;

class ExampleVulkanSystem : public GLFWVulkanSystem {
public:
    crimild::Bool start( void ) override
    {
        if ( !GLFWVulkanSystem::start() ) {
            return false;
        }

		m_frameGraph = crimild::alloc< FrameGraph >();

        m_scene = [&] {
            auto scene = crimild::alloc< Group >();

            scene->attachNode(
                [] {
                    auto earth = crimild::alloc< Group >();
                    earth->attachNode(
                        [] {
                            auto geometry = crimild::alloc< Geometry >();
                            geometry->attachPrimitive(
                                crimild::alloc< SpherePrimitive >(
                                    SpherePrimitive::Params {
                                        .type = Primitive::Type::TRIANGLES,
                                        .layout = VertexP3N3TC2::getLayout(),
                                    }
                                )
                            );
                            geometry->attachComponent< MaterialComponent >()->attachMaterial(
                                [&] {
                                    auto material = crimild::alloc< SimpleLitMaterial >();
                                    material->setAmbient( RGBAColorf( 0.05f, 0.05f, 0.1f, 1.0f ) );
                                    material->setDiffuseMap(
                                        [] {
                                            auto texture = crimild::alloc< Texture >();
                                            texture->imageView = [&] {
                                                auto imageView = crimild::alloc< ImageView >();
                                                imageView->image = [&] {
                                                    return ImageManager::getInstance()->loadImage(
                                                        {
                                                            .filePath = {
                                                                .path = "assets/textures/earth-diffuse.tga"
                                                            },
                                                        }
                                                    );
                                                }();
                                                return imageView;
                                            }();
                                            texture->sampler = crimild::alloc< Sampler >();
                                            return texture;
                                        }()
                                    );
                                    material->setSpecular( RGBAColorf( 0.95f, 0.95f, 1.0f, 1.0f ) );
                                    material->setSpecularMap(
                                        [] {
                                            auto texture = crimild::alloc< Texture >();
                                            texture->imageView = [&] {
                                                auto imageView = crimild::alloc< ImageView >();
                                                imageView->image = [&] {
                                                    return ImageManager::getInstance()->loadImage(
                                                        {
                                                            .filePath = {
                                                                .path = "assets/textures/earth-specular.tga"
                                                            },
                                                        }
                                                    );
                                                }();
                                                return imageView;
                                            }();
                                            texture->sampler = crimild::alloc< Sampler >();
                                            return texture;
                                        }()
                                    );
                                    material->setShininess( 4.0f );
                                    return material;
                                }()
                            );
                            return geometry;
                        }()
                    );
                    earth->attachComponent< RotationComponent >( Vector3f::UNIT_Y, 0.05f );
                    return earth;
                }()
            );

            scene->attachNode(
                [] {
                    auto group = crimild::alloc< Group >();
                    group->attachNode(
                        [] {
                            auto geometry = crimild::alloc< Geometry >();
                            geometry->attachPrimitive(
                                crimild::alloc< SpherePrimitive >(
                                    SpherePrimitive::Params {
                                        .type = Primitive::Type::TRIANGLES,
                                        .layout = VertexP3N3TC2::getLayout(),
                                        .radius = 0.1f,
                                    }
                                )
                            );
                            geometry->attachComponent< MaterialComponent >()->attachMaterial(
                                [] {
                                    auto material = crimild::alloc< UnlitMaterial >();
                                    material->setColor( RGBAColorf::ONE );
                                    return material;
                                }()
                            );
                            return geometry;
                        }()
                    );
                    group->attachNode(
                        [] {
                            auto light = crimild::alloc< Light >();
                            return light;
                        }()
                    );
                    group->attachComponent< LambdaComponent >(
                        [] ( auto node, auto &clock ) {
                            auto speed = 0.1f;
                            auto t = speed * clock.getCurrentTime();
                            auto x = Numericf::remapCos( -5.0f, 5.0f, t );
                            auto y = 0.0f;
                            auto z = Numericf::remapSin( -5.0f, 5.0f, t );
                            node->local().setTranslate( x, y, z );
                        }
                    );
                    return group;
                }()
            );

            scene->attachNode(
                [&] {
                    auto camera = crimild::alloc< Camera >();
                    camera->local().setTranslate( 0.0f, 0.0f, 3.0f );
                    Camera::setMainCamera( camera );
                    return camera;
                }()
            );

            return scene;
        }();

		m_renderPass = [&] {
            auto renderPass = crimild::alloc< RenderPass >();
            renderPass->attachments = {
                [&] {
                    auto att = crimild::alloc< Attachment >();
                    att->format = Format::COLOR_SWAPCHAIN_OPTIMAL;
                    return att;
                }(),
                [&] {
                    auto att = crimild::alloc< Attachment >();
                    att->format = Format::DEPTH_STENCIL_DEVICE_OPTIMAL;
                    return att;
                }()
            };

            renderPass->setDescriptors(
                [&] {
                    auto descriptorSet = crimild::alloc< DescriptorSet >();
                    descriptorSet->descriptors = {
                        Descriptor {
                            .descriptorType = DescriptorType::UNIFORM_BUFFER,
                            .obj = [&] {
                                FetchCameras fetch;
                                m_scene->perform( fetch );
                                auto camera = fetch.anyCamera();
                                return crimild::alloc< CameraViewProjectionUniform >( camera );
                            }(),
                        },
                        Descriptor {
                            .descriptorType = DescriptorType::UNIFORM_BUFFER,
                            .obj = [&] {
                                struct LightProps {
                                    Vector4f position;
                                };

                                FetchLights fetch;
                                m_scene->perform( fetch );

                                return crimild::alloc< DynamicUniformBuffer< LightProps >>(
                                    [ light = retain( fetch.anyLight() ) ] {
                                        auto p = light->getWorld().getTranslate();
                                        return LightProps {
                                            .position = Vector4f( p.x(), p.y(), p.z(), 0.0f ),
                                        };
                                    }
                                );
                            }(),
                        },
                    };
                    return descriptorSet;
                }()
            );
            renderPass->commands = [&] {
                auto commandBuffer = crimild::alloc< CommandBuffer >();
                m_scene->perform(
                    ApplyToGeometries(
                        [&]( Geometry *g ) {
                            if ( auto ms = g->getComponent< MaterialComponent >() ) {
                                if ( auto material = ms->first() ) {
                                    commandBuffer->bindGraphicsPipeline( material->getPipeline() );
                                    commandBuffer->bindDescriptorSet( renderPass->getDescriptors() );
                                    commandBuffer->bindDescriptorSet( material->getDescriptors() );
                                    commandBuffer->bindDescriptorSet( g->getDescriptors() );
                                    commandBuffer->drawPrimitive( g->anyPrimitive() );
                                }
                            }
						}
					)
				);
				return commandBuffer;
			}();
            renderPass->clearValue = {
                .color = RGBAColorf( 0.001235f, 0.01352941176, 0.0231372549f, 1.0f ),
            };
			return renderPass;
		}();

		m_master = [&] {
            auto master = crimild::alloc< PresentationMaster >();
            master->colorAttachment = m_renderPass->attachments[ 0 ];
			return master;
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
        m_renderPass = nullptr;
        m_master = nullptr;
        m_frameGraph = nullptr;

        GLFWVulkanSystem::stop();
    }

private:
    SharedPointer< Node > m_scene;
    SharedPointer< FrameGraph > m_frameGraph;
	SharedPointer< RenderPass > m_renderPass;
	SharedPointer< PresentationMaster > m_master;
};

int main( int argc, char **argv )
{
    crimild::init();
    crimild::vulkan::init();

    Log::setLevel( Log::Level::LOG_LEVEL_ALL );

    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "Lighting: Specular Map", crimild::alloc< Settings >( argc, argv ) );

    SharedPointer< ImageManager > imageManager = crimild::alloc< crimild::stb::ImageManager >();

    sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );

    return sim->run();
}
