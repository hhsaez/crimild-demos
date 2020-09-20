/*
 * Copyright (c) 2013, Hernan Saez
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
int main( int argc, char **argv )
{
	std::cout << "Usage: "
			  << "\nPress J to toggle color map"
		  	  << "\nPress K to toggle specular map"
		  	  << "\nPress L to toggle normal map" << std::endl;

	auto sim = crimild::alloc< SDLSimulation >( "A simple example", crimild::alloc< Settings >( argc, argv ) );

	float vertices[] = {
		-2.0f, +2.0f, 0.0f, 	0.0f, 0.0f, 1.0f,	1.0f, 0.0f, 0.0f,	0.0f, 1.0f,
		-2.0f, -2.0f, 0.0f, 	0.0f, 0.0f, 1.0f,	1.0f, 0.0f, 0.0f, 	0.0f, 0.0f,
		+2.0f, -2.0f, 0.0, 		0.0f, 0.0f, 1.0f,	1.0f, 0.0f, 0.0f,	1.0f, 0.0f,
		+2.0f, +2.0f, 0.0f,		0.0f, 0.0f, 1.0f, 	1.0f, 0.0f, 0.0f, 	1.0f, 1.0f
	};

	unsigned short indices[] = {
		0, 1, 2, 0, 2, 3
	};

	auto vbo = crimild::alloc< VertexBufferObject >( VertexFormat::VF_P3_N3_TG3_UV2, 4, vertices );
	auto ibo = crimild::alloc< IndexBufferObject >( 6, indices );

	auto primitive = crimild::alloc< Primitive >( Primitive::Type::TRIANGLES );
	primitive->setVertexBuffer( vbo );
	primitive->setIndexBuffer( ibo );

	auto geometry = crimild::alloc< Geometry >();
	geometry->attachPrimitive( primitive );

	auto material = crimild::alloc< Material >();
	auto colorMap = crimild::retain( AssetManager::getInstance()->get< Texture >( "stone-color.tga" ) );
	material->setColorMap( colorMap );
	auto specularMap = crimild::retain( AssetManager::getInstance()->get< Texture >( "stone-specular.tga" ) );
	material->setSpecularMap( specularMap );
	auto normalMap = crimild::retain( AssetManager::getInstance()->get< Texture >( "stone-normal.tga" ) );
	material->setNormalMap( normalMap );
	geometry->getComponent< MaterialComponent >()->attachMaterial( material );

	auto scene = crimild::alloc< Group >();
	scene->attachNode( geometry );

	auto interactiveLight = crimild::alloc< Group >();
	auto lightGeometry = crimild::alloc< Geometry >();
	lightGeometry->attachPrimitive( crimild::alloc< SpherePrimitive >( 0.025f, VertexFormat::VF_P3 ) );
	auto lightGeometryMaterial = crimild::alloc< Material >();
	lightGeometryMaterial->setProgram( AssetManager::getInstance()->get< ShaderProgram >( Renderer::SHADER_PROGRAM_UNLIT_DIFFUSE ) );
	lightGeometry->getComponent< MaterialComponent >()->attachMaterial( lightGeometryMaterial );
	interactiveLight->attachNode( lightGeometry );
	auto light = crimild::alloc< Light >();
	interactiveLight->attachNode( light );
	interactiveLight->local().setTranslate( 1.0f, 1.0f, 1.0f );
	scene->attachNode( interactiveLight );

	auto camera = crimild::alloc< Camera >( 45.0f, 4.0f / 3.0f, 0.1f, 1024.0f );
	camera->local().setTranslate( 0.0f, 0.0f, 6.0f );
	scene->attachNode( camera );

	scene->attachComponent( crimild::alloc< LambdaComponent >( [material, colorMap, normalMap, specularMap, interactiveLight, camera]( Node *, const Clock &t ) {
		if ( Input::getInstance()->isKeyDown( 'J' ) ) {
			material->setColorMap( material->getColorMap() != nullptr ? nullptr : colorMap );
		}

		if ( Input::getInstance()->isKeyDown( 'K' ) ) {
			material->setSpecularMap( material->getSpecularMap() != nullptr ? nullptr : specularMap );
		}

		if ( Input::getInstance()->isKeyDown( 'L' ) ) {
			material->setNormalMap( material->getNormalMap() != nullptr ? nullptr : normalMap );
		}

		if ( Input::getInstance()->isKeyDown( 'W' ) ) {
			interactiveLight->local().translate() += Vector3f( 0.0f, t.getDeltaTime(), 0.0f );
		}
		if ( Input::getInstance()->isKeyDown( 'S' ) ) {
			interactiveLight->local().translate() += Vector3f( 0.0f, -t.getDeltaTime(), 0.0f );
		}
		if ( Input::getInstance()->isKeyDown( 'A' ) ) {
			interactiveLight->local().translate() += Vector3f( -t.getDeltaTime(), 0.0f, 0.0f );
		}
		if ( Input::getInstance()->isKeyDown( 'D' ) ) {
			interactiveLight->local().translate() += Vector3f( t.getDeltaTime(), 0.0f, 0.0f );
		}

		if ( Input::getInstance()->isKeyDown( CRIMILD_INPUT_KEY_UP ) ) {
			camera->local().translate() += Vector3f( 0.0f, t.getDeltaTime(), 0.0f );
		}
		if ( Input::getInstance()->isKeyDown( CRIMILD_INPUT_KEY_DOWN ) ) {
			camera->local().translate() += Vector3f( 0.0f, -t.getDeltaTime(), 0.0f );
		}
		if ( Input::getInstance()->isKeyDown( CRIMILD_INPUT_KEY_LEFT ) ) {
			camera->local().translate() += Vector3f( -t.getDeltaTime(), 0.0f, 0.0f );
		}
		if ( Input::getInstance()->isKeyDown( CRIMILD_INPUT_KEY_RIGHT ) ) {
			camera->local().translate() += Vector3f( t.getDeltaTime(), 0.0f, 0.0f );
		}
		if ( Input::getInstance()->isKeyDown( '-' ) ) {
			camera->local().translate() += Vector3f( 0.0f, 0.0f, t.getDeltaTime() );
		}
		if ( Input::getInstance()->isKeyDown( '=' ) ) {
			camera->local().translate() += Vector3f( 0.0f, 0.0f, -t.getDeltaTime() );
		}
	}));

	sim->setScene( scene );
	return sim->run();
}
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

            scene->attachNode(
                [] {
                    auto geometry = crimild::alloc< Geometry >();
                    geometry->attachPrimitive( crimild::alloc< BoxPrimitive >( BoxPrimitive::Params { .invertFaces = true } ) );
                    geometry->attachComponent< MaterialComponent >()->attachMaterial(
                        [] {
                            auto loadTexture = []( auto fileName ) {
                                auto texture = crimild::alloc< Texture >();
                                texture->imageView = [&] {
                                    auto imageView = crimild::alloc< ImageView >();
                                    imageView->image = [&] {
                                        return ImageManager::getInstance()->loadImage(
                                            {
                                                .filePath = {
                                                    .path = fileName,
                                                },
                                            }
                                        );
                                    }();
                                    return imageView;
                                }();
                                texture->sampler = [&] {
                                    auto sampler = crimild::alloc< Sampler >();
                                    return sampler;
                                }();
                                return texture;
                            };

                            auto material = crimild::alloc< SimpleLitMaterial >();
                            material->setDiffuseMap( loadTexture( "stone-color.tga" ) );
                        	material->setSpecularMap( loadTexture( "stone-specular.tga" ) );
                            material->setNormalMap( loadTexture( "stone-normal.tga" ) );
                            return material;
                        }() );
                	geometry->attachComponent< RotationComponent >( Vector3f::ONE.getNormalized(), 0.01 );
                    return geometry;
                }() );

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
                                        .radius = 0.05f,
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
                            light->setAttenuation( Vector3f( 1.0f, 0.7f, 0.8f ) );
                            return light;
                        }()
                    );
                    group->attachComponent< LambdaComponent >(
                        [ t = 0.0f ] ( auto node, auto &clock ) mutable {
                            auto speed = 0.005f;
                            auto x = Numericf::remap( -1.0f, 1.0f, -0.95f, 0.95f, Numericf::cos( t ) * Numericf::sin( t ) );
                            auto y = Numericf::remapSin( -0.95f, 0.95f, t );
                            auto z = Numericf::remapCos( -0.95f, 0.95f, t );;
                            if ( !Input::getInstance()->isKeyDown( CRIMILD_INPUT_KEY_SPACE ) ) {
                                node->local().setTranslate( x, y, z );
                                t += speed + clock.getDeltaTime();
                            }
                        }
                    );
                    return group;
                }()
            );

            scene->attachNode(
                [ & ] {
                    auto camera = crimild::alloc< Camera >();
                    camera->local().setTranslate( 0.0f, 0.0f, 3.0f );
                    camera->local().lookAt( Vector3f::ZERO );
                    return camera;
                }() );

            scene->perform( StartComponents() );

            return scene;
        }();

        m_composition = [ & ] {
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

    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "Normal Mapping", crimild::alloc< Settings >( argc, argv ) );

    SharedPointer< ImageManager > imageManager = crimild::alloc< crimild::stb::ImageManager >();

    sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );

    return sim->run();
}
