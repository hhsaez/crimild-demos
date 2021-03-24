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
            [] {
                auto scene = crimild::alloc< Group >();

                auto createPrimitive = [] {
                    auto primitive = crimild::alloc< Primitive >( Primitive::Type::TRIANGLES );
                    primitive->setVertexData(
                        {
                            [] {
                                return crimild::alloc< VertexBuffer >(
                                    VertexP3C3::getLayout(),
                                    Array< VertexP3C3 > {
                                        {
                                            .position = Vector3f( -0.5f, -0.5f, 0.0f ),
                                            .color = RGBColorf( 1.0f, 0.0f, 0.0f ),
                                        },
                                        {
                                            .position = Vector3f( 0.5f, -0.5f, 0.0f ),
                                            .color = RGBColorf( 0.0f, 1.0f, 0.0f ),
                                        },
                                        {
                                            .position = Vector3f( 0.0f, 0.5f, 0.0f ),
                                            .color = RGBColorf( 0.0f, 0.0f, 1.0f ),
                                        },
                                    } );
                            }(),
                        } );
                    primitive->setIndices(
                        crimild::alloc< IndexBuffer >(
                            Format::INDEX_32_UINT,
                            Array< crimild::UInt32 > {
                                0,
                                1,
                                2,
                            } ) );
                    return primitive;
                };

                auto createMaterial = [] {
                    auto material = crimild::alloc< UnlitMaterial >();
                    material->setGraphicsPipeline(
                        [] {
                            auto pipeline = crimild::alloc< GraphicsPipeline >();
                            pipeline->setProgram(
                                [] {
                                    auto program = crimild::alloc< UnlitShaderProgram >();
                                    program->setShaders(
                                        Array< SharedPointer< Shader > > {
                                            Shader::withSource(
                                                Shader::Stage::VERTEX,
                                                FilePath { .path = "assets/shaders/scene.vert" } ),
                                            Shader::withSource(
                                                Shader::Stage::FRAGMENT,
                                                FilePath { .path = "assets/shaders/scene.frag" } ),
                                        } );
                                    program->vertexLayouts = { VertexLayout::P3_C3 };
                                    return program;
                                }() );
                            return pipeline;
                        }() );
                    return material;
                };

                auto createTriangle = [ primitive = createPrimitive(),
                                        material = createMaterial() ]( const Transformation &pose ) {
                    auto geometry = crimild::alloc< Geometry >();
                    geometry->attachPrimitive( primitive );
                    geometry->attachComponent< MaterialComponent >( material );
                    geometry->setLocal( pose );
                    return geometry;
                };

                auto rnd = Random::Generator( 1982 );

                for ( auto x = -10.0f; x <= 10.0f; x += 1.0f ) {
                    for ( auto z = -10.0f; z <= 10.0f; z += 1.0f ) {
                        scene->attachNode(
                            createTriangle(
                                Transformation(
                                    Vector3f( x, 0.0f, z + ( 0.1f * x / 10.0f ) ),
                                    Quaternion4f::createFromAxisAngle(
                                        Vector3f(
                                            rnd.generate(),
                                            rnd.generate(),
                                            rnd.generate() )
                                            .getNormalized(),
                                        rnd.generate( 0.0f, Numericf::TWO_PI ) ),
                                    rnd.generate( 0.5f, 1.5f ) ) ) );
                    }
                }

                scene->attachNode( [] {
                    auto camera = crimild::alloc< Camera >();
                    camera->local().setTranslate( 5.0f, 10.0f, 10.0f );
                    camera->local().lookAt( -Vector3f::UNIT_Y );
                    Camera::setMainCamera( camera );
                    return camera;
                }() );
                return scene;
            }() );
    }
};

CRIMILD_CREATE_SIMULATION( Example, "Transformations" );
