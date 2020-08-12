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
using namespace crimild::compositions;
using namespace crimild::glfw;

class ExampleVulkanSystem : public GLFWVulkanSystem {
public:
    crimild::Bool start( void ) override
    {
        if ( !GLFWVulkanSystem::start() ) {
            return false;
        }

		m_frameGraph = crimild::alloc< FrameGraph >();

        auto rnd = Random::Generator( 1982 );

        m_scene = [&] {
            auto scene = crimild::alloc< Group >();

            auto primitive = crimild::alloc< BoxPrimitive >(
                BoxPrimitive::Params {
                    .type = Primitive::Type::TRIANGLES,
                    .layout = VertexP3N3TC2::getLayout(),
                }
            );


            auto material = [] {
                auto material = crimild::alloc< SimpleLitMaterial >(
                    SimpleLitMaterial::Props {
                        .ambient = RGBAColorf( 0.0215f, 0.1745f, 0.0215f, 1.0f ),
                        .diffuse = RGBAColorf( 0.07568f, 0.61424f, 0.07568f, 1.0f ),
                        .specular = RGBAColorf( 0.633f, 0.727811f, 0.633f, 1.0f ),
                        .shininess = 128.0f * 0.6f
                    }
                );
                material->setDiffuseMap(
                    [] {
                        auto texture = crimild::alloc< Texture >();
                        texture->imageView = [] {
                            auto imageView = crimild::alloc< ImageView >();
                            imageView->image = Image::CHECKERBOARD_4;
                            return imageView;
                        }();
                        texture->sampler = [] {
                            auto sampler = crimild::alloc< Sampler >();
                            sampler->setMinFilter( Sampler::Filter::NEAREST );
                            sampler->setMagFilter( Sampler::Filter::NEAREST );
                            return sampler;
                        }();
                        return texture;
                    }()
                );
                return material;
            }();

            for ( auto i = 0; i < 30; ++i ) {
                scene->attachNode(
                    [&] {
                        auto geometry = crimild::alloc< Geometry >();
                        geometry->attachPrimitive( primitive );

                        geometry->local().setTranslate(
                            rnd.generate( -10.0f, 10.0f ),
                            rnd.generate( -10.0f, 10.0f ),
                            rnd.generate( -10.0f, 10.0f )
                        );

                        geometry->local().setScale( rnd.generate( 0.75f, 1.5f ) );

                        geometry->local().rotate().fromAxisAngle(
                            Vector3f(
                                rnd.generate( 0.01f, 1.0f ),
                                rnd.generate( 0.01f, 1.0f ),
                                rnd.generate( 0.01f, 1.0f )
                            ).getNormalized(),
                            rnd.generate( 0.0f, Numericf::TWO_PI )
                        );

                        geometry->attachComponent< MaterialComponent >()->attachMaterial( material );

                        return geometry;
                    }()
                );
            }

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
                            auto light = crimild::alloc< Light >(
                                Light::Type::POINT
                            );
                            light->setAttenuation( Vector3f( 1.0f, 0.027f, 0.0028f ) );
                            light->setAmbient( RGBAColorf::ONE );
                            return light;
                        }()
                    );
                    group->attachComponent< LambdaComponent >(
                        [] ( auto node, auto &clock ) {
                            auto speed = 0.25f;
                            auto t = speed * clock.getCurrentTime();
                            auto x = Numericf::remap( -1.0f, 1.0f, -15.0f, 15.0f, Numericf::cos( t ) * Numericf::sin( t ) );
                            auto y = Numericf::remapSin( -3.0f, 3.0f, t );
                            auto z = Numericf::remapCos( -15.0f, 15.0f, t );;
                            node->local().setTranslate( x, y, z );
                        }
                    );
                    return group;
                }()
            );

            scene->attachNode(
                [&] {
                    auto camera = crimild::alloc< Camera >();
                    camera->local().setTranslate( 0.0f, 0.0f, 30.0f );
                    return camera;
                }()
            );

            scene->perform( StartComponents() );

            return scene;
        }();

        m_composition = present(
            sharpen(
                renderScene( crimild::get_ptr( m_scene ) )
            )
        );

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
    SharedPointer< RenderPass > m_screenPass;
	SharedPointer< PresentationMaster > m_master;
    Composition m_composition;
};

int main( int argc, char **argv )
{
    crimild::init();
    crimild::vulkan::init();

    Log::setLevel( Log::Level::LOG_LEVEL_ALL );

    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "Postprocessing: Sharpen", crimild::alloc< Settings >( argc, argv ) );

    SharedPointer< ImageManager > imageManager = crimild::alloc< crimild::stb::ImageManager >();

    sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );

    return sim->run();
}
