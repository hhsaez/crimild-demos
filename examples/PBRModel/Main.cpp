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
#include <Crimild_GLFW.hpp>
#include <Crimild_STB.hpp>
#include <Crimild_Vulkan.hpp>

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

        m_scene = [ & ] {
            auto scene = crimild::alloc< Group >();

            scene->attachNode( [ & ] {
                auto path = FilePath {
                    .path = "assets/models/cerberus/cerberus.obj"
                };
                auto group = crimild::alloc< Group >();
                OBJLoader loader( path.getAbsolutePath() );
                loader.setVerbose( true );
                if ( auto model = loader.load() ) {
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
                        auto texture = crimild::alloc< Texture >();
                        texture->imageView = [] {
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
        }();

        m_composition = [ & ] {
            using namespace crimild::compositions;
            auto enableTonemapping = true;
            auto enableBloom = false;
            auto enableDebug = true;

            auto withTonemapping = [ enableTonemapping ]( auto cmp ) {
                return enableTonemapping ? tonemapping( cmp, 1.0 ) : cmp;
            };

            auto withDebug = [ enableDebug ]( auto cmp ) {
                return enableDebug ? debug( cmp ) : cmp;
            };

            auto withBloom = [ enableBloom ]( auto cmp ) {
                return enableBloom ? bloom( cmp ) : cmp;
            };

            return present( withDebug( withTonemapping( withBloom( renderSceneHDR( m_scene ) ) ) ) );
        }();

        if ( m_frameGraph->compile() ) {
            auto commands = m_frameGraph->recordCommands();
            setCommandBuffers( { commands } );
        }

        return true;
    }

    void
    update( void ) override
    {
        auto clock = Simulation::getInstance()->getSimulationClock();

        auto updateScene = [ & ]( auto &scene ) {
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
    settings->set( "video.width", 1280 );
    settings->set( "video.height", 900 );
    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "PBR: 3D Model", settings );

    SharedPointer< ImageManager > imageManager = crimild::alloc< crimild::stb::ImageManager >();

    sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );

    return sim->run();
}
