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

#include "Rendering/ReflectionMaterial.hpp"
#include "Rendering/RefractionMaterial.hpp"

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

            auto environmentTexture = [] {
                auto texture = crimild::alloc< Texture >();
                texture->imageView = [&] {
                    auto imageView = crimild::alloc< ImageView >();
                    imageView->image = ImageManager::getInstance()->loadCubemap(
                        {
                            .filePaths = {
                                { .path = "assets/textures/right.png" },
                                { .path = "assets/textures/left.png" },
                                { .path = "assets/textures/top.png" },
                                { .path = "assets/textures/bottom.png" },
                                { .path = "assets/textures/back.png" },
                                { .path = "assets/textures/front.png" },
                            },
                        }
                    );
                    return imageView;
                }();
                texture->sampler = [&] {
                    auto sampler = crimild::alloc< Sampler >();
                    sampler->setMinFilter( Sampler::Filter::NEAREST );
                    sampler->setMagFilter( Sampler::Filter::NEAREST );
                    sampler->setWrapMode( Sampler::WrapMode::CLAMP_TO_BORDER );
                    sampler->setCompareOp( CompareOp::NEVER );
                    return sampler;
                }();
                return texture;
            }();

            auto reflectionMaterial = [&] () -> SharedPointer< Material > {
                auto material = crimild::alloc< ReflectionMaterial >();
                material->setTexture( environmentTexture );
                return material;
            }();

            auto refractionMaterial = [&] () -> SharedPointer< Material > {
                auto material = crimild::alloc< RefractionMaterial >();
                material->setTexture( environmentTexture );
                return material;
            }();

            math::fibonacciSquares( 20 ).each(
                [&, i = 0]( auto &it ) mutable {
                    scene->attachNode(
                        [&] {
                            auto geometry = crimild::alloc< Geometry >();
                            geometry->attachPrimitive(
                                crimild::alloc< ParametricSpherePrimitive >(
                                    ParametricSpherePrimitive::Params {
                                        .type = Primitive::Type::TRIANGLES,
                                        .layout = VertexP3N3::getLayout(),
                                    }
                                )
                            );
                            geometry->local().setTranslate( it.first + Vector3f::UNIT_Z * it.second );
                            geometry->local().setScale( 0.25f * it.second );
                            auto material = ( i++ % 2 == 0 ? reflectionMaterial : refractionMaterial );
                            geometry->attachComponent< MaterialComponent >()->attachMaterial( material );
                            return geometry;
                        }()
                    );
                }
            );

            scene->attachNode(
                [&] {
                    return crimild::alloc< Skybox >( environmentTexture );
                }()
            );

            scene->attachNode([] {
                auto camera = crimild::alloc< Camera >();
                camera->local().setTranslate( 0.0f, 10.0f, 100.0f );
                camera->local().lookAt( Vector3f::ZERO );
                camera->attachComponent< FreeLookCameraComponent >();
                Camera::setMainCamera( camera );
                return camera;
            }());

            scene->perform( StartComponents() );

            return scene;
        }();

        m_composition = [&] {
            using namespace crimild::compositions;
            return present( renderScene( m_scene ) );
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
        m_frameGraph = nullptr;

        GLFWVulkanSystem::stop();
    }

private:
    SharedPointer< Node > m_scene;
    SharedPointer< FrameGraph > m_frameGraph;
    compositions::Composition m_composition;
};

int main( int argc, char **argv )
{
    crimild::init();
    crimild::vulkan::init();

    Log::setLevel( Log::Level::LOG_LEVEL_ALL );

    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "Environment Map: Reflection & Refraction", crimild::alloc< Settings >( argc, argv ) );

    SharedPointer< ImageManager > imageManager = crimild::alloc< crimild::stb::ImageManager >();

    sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );
    return sim->run();
}
