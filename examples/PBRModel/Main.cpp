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
        setScene( [ & ] {
            auto scene = crimild::alloc< Group >();

            auto loadTexture = []( std::string path ) {
                auto texture = crimild::alloc< Texture >();
                texture->imageView = [ path ] {
                    auto imageView = crimild::alloc< ImageView >();
                    imageView->image = ImageManager::getInstance()->loadImage(
                        {
                            .filePath = {
                                .path = path,
                            },
                        } );
                    return imageView;
                }();
                texture->sampler = [ & ] {
                    auto sampler = crimild::alloc< Sampler >();
                    sampler->setMinFilter( Sampler::Filter::LINEAR );
                    sampler->setMagFilter( Sampler::Filter::LINEAR );
                    return sampler;
                }();
                return texture;
            };

            auto material = crimild::alloc< LitMaterial >();
            material->setAlbedoMap( loadTexture( "assets/models/cerberus/Cerberus_A.tga" ) );
            material->setMetallicMap( loadTexture( "assets/models/cerberus/Cerberus_M.tga" ) );
            material->setRoughnessMap( loadTexture( "assets/models/cerberus/Cerberus_R.tga" ) );
            material->setNormalMap( loadTexture( "assets/models/cerberus/Cerberus_N.tga" ) );

            scene->attachNode( [ & ] {
                auto path = FilePath {
                    .path = "assets/models/cerberus/cerberus.obj"
                };
                auto group = crimild::alloc< Group >();
                OBJLoader loader( path.getAbsolutePath() );
                loader.setVerbose( true );
                if ( auto model = loader.load() ) {
                    model->perform(
                        Apply(
                            [ material ]( Node *node ) {
                                if ( auto ms = node->getComponent< MaterialComponent >() ) {
                                    ms->detachAllMaterials();
                                    ms->attachMaterial( material );
                                }
                            } ) );
                    group->attachNode( model );
                    group->local().rotate().fromAxisAngle( Vector3f::UNIT_Y, 0.5f );
                    group->local().setScale( 10.0f );
                }
                return group;
            }() );

            scene->attachNode(
                crimild::alloc< Skybox >(
                    [] {
                        auto imageWithRGBA = []( auto r, auto g, auto b, auto a ) {
                            auto image = crimild::alloc< Image >();
                            image->extent = {
                                .width = 1,
                                .height = 1,
                                .depth = 1,
                            };
                            image->format = Format::R8G8B8A8_UNORM;
                            image->data = {
                                UInt8( r * 255 ),
                                UInt8( g * 255 ),
                                UInt8( b * 255 ),
                                UInt8( a * 255 ),
                            };
                            return image;
                        };
                        auto texture = crimild::alloc< Texture >();
                        texture->imageView = [ imageWithRGBA ] {
                            auto imageView = crimild::alloc< ImageView >();
                            imageView->image = ImageManager::getInstance()->loadImage(
                                {
                                    .filePath = {
                                        .path = "assets/textures/Newport_Loft_Ref.hdr",
                                    },
                                    .hdr = true,
                                } );
                            return imageView;
                        }();
                        texture->sampler = [ & ] {
                            auto sampler = crimild::alloc< Sampler >();
                            sampler->setMinFilter( Sampler::Filter::LINEAR );
                            sampler->setMagFilter( Sampler::Filter::LINEAR );
                            sampler->setWrapMode( Sampler::WrapMode::CLAMP_TO_BORDER );
                            sampler->setCompareOp( CompareOp::NEVER );
                            return sampler;
                        }();
                        return texture;
                    }() ) );

            auto createLight = []( const auto &position ) {
                auto light = crimild::alloc< Light >( Light::Type::POINT );
                light->local().setTranslate( position );
                light->setColor( RGBAColorf( 55, 55, 55 ) );
                return light;
            };

            scene->attachNode( createLight( Vector3f( -15.0f, +15.0f, 10.0f ) ) );
            scene->attachNode( createLight( Vector3f( +15.0f, +15.0f, 10.0f ) ) );
            scene->attachNode( createLight( Vector3f( -15.0f, -15.0f, 10.0f ) ) );
            scene->attachNode( createLight( Vector3f( +15.0f, -15.0f, 10.0f ) ) );

            scene->attachNode(
                [ & ] {
                    auto camera = crimild::alloc< Camera >();
                    camera->local().setTranslate( 0.0f, 0.0f, 30.0f );
                    camera->attachComponent< FreeLookCameraComponent >();
                    return camera;
                }() );

            scene->perform( StartComponents() );

            return scene;
        }() );
    }
};

CRIMILD_CREATE_SIMULATION( Example, "PBR: Model" );
