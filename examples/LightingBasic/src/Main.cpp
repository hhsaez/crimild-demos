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
        setScene(
            [ & ] {
                auto scene = crimild::alloc< Group >();

                auto createMaterial = []( auto program ) {
                    auto material = crimild::alloc< SimpleLitMaterial >();
                    material->getGraphicsPipeline()->setProgram( crimild::retain( program ) );
                    material->setDiffuse( RGBAColorf( 1.0f, 0.1f, 0.1f, 1.0f ) );
                    return material;
                };

                auto gouraudMaterial = createMaterial( AssetManager::getInstance()->get< GouraudLitShaderProgram >() );
                auto phongMaterial = createMaterial( AssetManager::getInstance()->get< PhongLitShaderProgram >() );

                auto sphere = []( auto position, auto material ) {
                    auto geometry = crimild::alloc< Geometry >();
                    geometry->attachPrimitive(
                        crimild::alloc< SpherePrimitive >(
                            SpherePrimitive::Params {
                                .type = Primitive::Type::TRIANGLES,
                                .layout = VertexP3N3TC2::getLayout(),
                            } ) );
                    geometry->attachComponent< MaterialComponent >()->attachMaterial( material );
                    geometry->local().setTranslate( position );
                    return geometry;
                };

                scene->attachNode( sphere( Vector3f( -1.25f, 0.0f, 0.0f ), gouraudMaterial ) );
                scene->attachNode( sphere( Vector3f( 1.25f, 0.0f, 0.0f ), phongMaterial ) );

                scene->attachNode(
                    [] {
                        auto light = crimild::alloc< Light >( Light::Type::DIRECTIONAL );
                        light->local().setTranslate( 10.0f, 10.0f, 10.0f );
                        light->local().lookAt( Vector3f::ZERO );
                        return light;
                    }() );

                scene->attachNode( [] {
                    auto camera = crimild::alloc< Camera >();
                    camera->local().setTranslate( 0.0f, 0.0f, 5.0f );
                    camera->local().lookAt( Vector3f::ZERO );
                    Camera::setMainCamera( camera );
                    return camera;
                }() );

                scene->perform( StartComponents() );

                return scene;
            }() );

        // m_composition = [ & ] {
        //     using namespace crimild::compositions;
        //     return present( renderScene( m_scene ) );
        // }();
    }
};

CRIMILD_CREATE_SIMULATION( Example, "Lighting: Basic" );
