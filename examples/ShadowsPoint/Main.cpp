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

        m_frameGraph = crimild::alloc< FrameGraph >();

        m_scene = [&] {
            auto scene = crimild::alloc< Group >();

            scene->attachNode(
                [] {
                    SharedPointer< Primitive > primitives[ 6 ] = {
                        crimild::alloc< BoxPrimitive >(
                            BoxPrimitive::Params {
                                .type = Primitive::Type::TRIANGLES,
                                .layout = VertexP3N3TC2::getLayout(),
                            }
                        ),
                        crimild::alloc< SpherePrimitive >(
                            SpherePrimitive::Params {
                                .type = Primitive::Type::TRIANGLES,
                                .layout = VertexP3N3TC2::getLayout(),
                            }
                        ),
                        crimild::alloc< TorusPrimitive >(
                            TorusPrimitive::Params {
                                .type = Primitive::Type::TRIANGLES,
                                .layout = VertexP3N3TC2::getLayout(),
                            }
                        ),
                        crimild::alloc< ConePrimitive >(
                            ConePrimitive::Params {
                                .type = Primitive::Type::TRIANGLES,
                                .layout = VertexP3N3TC2::getLayout(),
                            }
                        ),
                        crimild::alloc< CylinderPrimitive >(
                            CylinderPrimitive::Params {
                                .type = Primitive::Type::TRIANGLES,
                                .layout = VertexP3N3TC2::getLayout(),
                            }
                        ),
                        crimild::alloc< TrefoilKnotPrimitive >(
                            TrefoilKnotPrimitive::Params {
                                .type = Primitive::Type::TRIANGLES,
                                .layout = VertexP3N3TC2::getLayout(),
                            }
                        ),
                    };

                    auto material = [] {
                        auto material = crimild::alloc< SimpleLitMaterial >(
                            SimpleLitMaterial::Props {
                                .ambient = RGBAColorf( 0.0215f, 0.1745f, 0.0215f, 1.0f ),
                                .diffuse = RGBAColorf( 0.07568f, 0.61424f, 0.07568f, 1.0f ),
                                .specular = RGBAColorf( 0.633f, 0.727811f, 0.633f, 1.0f ),
                                .shininess = 128.0f * 0.6f
                            }
                        );
                        return material;
                    }();

                    auto group = crimild::alloc< Group >();
                    auto rnd = Random::Generator( 1982 );
                    for ( auto i = 0; i < 60; ++i ) {
                        group->attachNode(
                            [&] {
                                auto geometry = crimild::alloc< Geometry >();
                                geometry->attachPrimitive( primitives[ i % 6 ] );

                                geometry->local().setTranslate(
                                    rnd.generate( -15.0f, 15.0f ),
                                    rnd.generate( -15.0f, 15.0f ),
                                    rnd.generate( -15.0f, 15.0f )
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
                    return group;
                }()
            );

            scene->attachNode(
                [] {
                    auto primitive = crimild::alloc< BoxPrimitive >(
                        BoxPrimitive::Params {
                            .type = Primitive::Type::TRIANGLES,
                            .layout = VertexP3N3TC2::getLayout(),
                            .size = 20.0f * Vector3f::ONE,
                            .invertFaces = true,
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
                        return material;
                    }();

                    auto geometry = crimild::alloc< Geometry >();
                    geometry->attachPrimitive( primitive );
                    geometry->attachComponent< MaterialComponent >()->attachMaterial( material );
                    return geometry;
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
                                    material->setCastShadows( false );
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
                            light->setAttenuation( Vector3f( 1.0f, 0.007f, 0.0002f ) );
                            light->setAmbient( RGBAColorf::ONE );
                            light->setCastShadows( true );
                            return light;
                        }()
                    );
                    group->attachComponent< LambdaComponent >(
                        [] ( auto node, auto &clock ) {
                            auto speed = 0.125f;
                            auto t = speed * clock.getCurrentTime();
                            auto x = Numericf::remap( -1.0f, 1.0f, -15.0f, 15.0f, Numericf::cos( t ) * Numericf::sin( t ) );
                            auto y = Numericf::remapSin( -3.0f, 3.0f, t );
                            auto z = Numericf::remapCos( -15.0f, 15.0f, t );;
                            if ( !Input::getInstance()->isKeyDown( CRIMILD_INPUT_KEY_SPACE ) ) {
                                node->local().setTranslate( x, y, z );
                            }
                        }
                    );
                    return group;
                }()
            );

            scene->attachNode([] {
                auto camera = crimild::alloc< Camera >();
                camera->local().setTranslate( 15.0f, 5.0f, 40.0f );
                camera->local().lookAt( 1.0 * Vector3f::UNIT_Y );
                camera->attachComponent< FreeLookCameraComponent >();
                return camera;
            }());

            scene->perform( StartComponents() );

            return scene;
        }();

        m_composition = [&] {
            using namespace crimild::compositions;
            return present( debug( renderScene( m_scene ) ) );
            //return present( renderScene( m_scene ) );
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

        auto updateScene = [&]( auto &scene ) {
            scene->perform( UpdateComponents( clock ) );
            scene->perform( UpdateWorldState() );
        };

        updateScene( m_scene );

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
    SharedPointer< FrameGraph > m_frameGraph;
    SharedPointer< Node > m_scene;
    compositions::Composition m_composition;
};

int main( int argc, char **argv )
{
    crimild::init();
    crimild::vulkan::init();

    Log::setLevel( Log::Level::LOG_LEVEL_ALL );

    auto settings = crimild::alloc< Settings >( argc, argv );
    //settings->set( "video.width", 720 );
    //settings->set( "video.height", 720 );

    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "Shadows: Point Light", settings );

    SharedPointer< ImageManager > imageManager = crimild::alloc< crimild::stb::ImageManager >();

    sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );

    return sim->run();
}
