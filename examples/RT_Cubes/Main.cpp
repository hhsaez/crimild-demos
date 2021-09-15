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

using namespace crimild;

class Example : public Simulation {
public:
    void onStarted( void ) noexcept override
    {
        const auto useRaster = Simulation::getInstance()->getSettings()->get< Bool >( "use_raster", false );
        const auto useCompute = Simulation::getInstance()->getSettings()->get< Bool >( "use_compute", true );

        setScene(
            [ & ] {
                auto scene = crimild::alloc< Group >();

                auto fromRGB = []( Real r, Real g, Real b ) {
                    return ColorRGB { r / 255.0f, g / 255.0f, b / 255.0f };
                };

                auto withRGB = [ fromRGB ]( auto r, auto g, auto b ) {
                    auto material = crimild::alloc< materials::PrincipledBSDF >();
                    material->setAlbedo( fromRGB( r, g, b ) );
                    return material;
                };

                auto emissive = []( Real r, Real g, Real b ) {
                    auto material = crimild::alloc< materials::PrincipledBSDF >();
                    material->setEmissive( ColorRGB { r, g, b } );
                    return material;
                };

                auto materials = Array< SharedPointer< Material > > {
                    withRGB( 48, 49, 45 ),
                    withRGB( 142, 136, 123 ),
                    withRGB( 238, 227, 222 ),
                    emissive( 2, 0.75, 0.75 ),
                    emissive( 2, 2, 0.75 ),
                    emissive( 0.75, 2, 2 ),
                    withRGB( 254, 86, 102 ),
                    withRGB( 253, 202, 85 ),
                    withRGB( 158, 206, 220 ),
                    withRGB( 129, 128, 199 ),
                    withRGB( 254, 153, 187 ),
                    withRGB( 95, 103, 135 ),
                    withRGB( 253, 152, 53 )
                };

                auto box = []( const auto &center, const auto &size, auto material ) -> SharedPointer< Node > {
                    auto geometry = crimild::alloc< Geometry >();
                    geometry->attachPrimitive( crimild::alloc< Primitive >( Primitive::Type::BOX ) );
                    geometry->setLocal( translation( vector3( center ) ) * scale( size.x, size.y, size.z ) );
                    geometry->attachComponent< MaterialComponent >( material );
                    return geometry;
                };

                const auto boxesPerSide = 10.0f;
                Array< SharedPointer< Node > > boxes( ( 2 * boxesPerSide + 1 ) * ( 2 * boxesPerSide + 1 ) * ( 2 * boxesPerSide + 1 ) );
                Size boxId = 0;
                for ( auto x = -boxesPerSide; x <= boxesPerSide; ++x ) {
                    for ( auto y = -boxesPerSide; y <= boxesPerSide; ++y ) {
                        for ( auto z = -boxesPerSide; z <= boxesPerSide; ++z ) {
                            auto size = Vector3 {
                                Random::generate< Real >( 0.25, 3.0 ),
                                Random::generate< Real >( 0.25, 3.0 ),
                                Random::generate< Real >( 0.25, 3.0 ),
                            };
                            auto offset = Vector3 {
                                Random::generate< Real >( 0.5 * x, 5.0 * x ),
                                Random::generate< Real >( 0.5 * y, 5.0 * y ),
                                Random::generate< Real >( 0.5 * z, 5.0 * z ),
                            };
                            boxes[ boxId++ ] = box(
                                Point3 { x, y, z } + offset,
                                size,
                                materials[ Random::generate< Int >( 0, materials.size() ) ] );
                        }
                    }
                }

                if ( useRaster ) {
                    // TODO: make the RT take the background color from the skybox
                    scene->attachNode( crimild::alloc< Skybox >( ColorRGB { 0.5f, 0.6f, 0.7f } ) );
                }

                Simulation::getInstance()->getSettings()->set( "rt.background_color.r", 0.5f );
                Simulation::getInstance()->getSettings()->set( "rt.background_color.g", 0.6f );
                Simulation::getInstance()->getSettings()->set( "rt.background_color.b", 0.7f );

                scene->attachNode( framegraph::utils::optimize( boxes ) );

                scene->attachNode( [] {
                    auto camera = crimild::alloc< Camera >( 20.0f, 4.0f / 3.0f, 0.1f, 1024.0f );
                    camera->setLocal(
                        lookAt(
                            Point3 { 250, 250, 250 }, //70, 70, 70 },
                            Point3 { 0, 0, 0 },       //10, 50, 0 },
                            Vector3::Constants::UP ) );
                    camera->attachComponent< FreeLookCameraComponent >();
                    return camera;
                }() );

                scene->perform( StartComponents() );

                return scene;
            }() );

        if ( !useRaster ) {
            RenderSystem::getInstance()->setFrameGraph(
                [ & ] {
                    using namespace crimild::framegraph;
                    return present( tonemapping( useResource( useCompute ? computeRT() : softRT() ) ) );
                }() );
        }
    }
};

CRIMILD_CREATE_SIMULATION( Example, "RT: Cubes" );