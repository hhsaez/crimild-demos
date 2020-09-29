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

            auto primitive = crimild::alloc< SpherePrimitive >(
                SpherePrimitive::Params {
                    .type = Primitive::Type::TRIANGLES,
                    .layout = VertexP3N3TC2::getLayout(),
                } );

            for ( auto y = 0; y < 7; ++y ) {
                for ( auto x = 0; x < 7; ++x ) {
                    auto geometry = crimild::alloc< Geometry >();
                    geometry->attachPrimitive( primitive );
                    geometry->local().setTranslate( 2.5f * Vector3f( -3.0f + x, 3.0f - y, 0 ) );
                    geometry->attachComponent< MaterialComponent >()->attachMaterial(
                        [ x, y ] {
                            auto material = crimild::alloc< LitMaterial >();
                            material->setAlbedo( RGBColorf( 1.0f, 0.0f, 0.0f ) );
                            material->setMetallic( 1.0f - float( y ) / 6.0f );
                            material->setRoughness( float( x ) / 6.0f );
                            return material;
                        }() );
                    scene->attachNode( geometry );
                }
            }

            auto createLight = []( const auto &position ) {
                auto light = crimild::alloc< Light >( Light::Type::POINT );
                light->local().setTranslate( position );
                light->setColor( RGBAColorf( 55, 55, 55 ) );
                return light;
            };

            scene->attachNode(
                [] {
                    auto geometry = crimild::alloc< Geometry >();
                    geometry->attachPrimitive( crimild::alloc< BoxPrimitive >() );
                    geometry->attachComponent< MaterialComponent >()->attachMaterial(
                        [] {
                            auto material = crimild::alloc< Material >();
                            material->setDescriptors(
                                [&] {
                                    auto descriptors = crimild::alloc< DescriptorSet >();
                                    descriptors->descriptors = {
                                        {
                                            .descriptorType = DescriptorType::TEXTURE,
                                            .obj = [] {
                                                auto texture = crimild::alloc< Texture >();
                                                texture->imageView = [] {
                                                    auto imageView = crimild::alloc< ImageView >();
                                                    imageView->image = ImageManager::getInstance()->loadImage(
                                                        {
                                                            .filePath = {
                                                                .path = "assets/textures/Newport_Loft_Ref.hdr",
                                                            },
                                                            .hdr = true,
                                                        }
                                                    );
                                                    return imageView;
                                                }();
                                                texture->sampler = [ & ] {
                                                    auto sampler = crimild::alloc< Sampler >();
                                                    sampler->setMinFilter( Sampler::Filter::LINEAR );
                                                    sampler->setMagFilter( Sampler::Filter::LINEAR );
                                                    return sampler;
                                                }();
                                                return texture;
                                            }(),
                                        },
                                    };
                                    return descriptors;
                                }()
                            );
                        	material->setPipeline(
                                [] {
                                    auto pipeline = crimild::alloc< Pipeline >();
                                    pipeline->program = [] {
                                        auto program = crimild::alloc< ShaderProgram >();
                                        program->setShaders(
                                            {
                                                crimild::alloc< Shader >(
                                                    Shader::Stage::VERTEX,
                                                	R"(
                                                        layout ( location = 0 ) in vec3 inPosition;
                                                        layout ( location = 1 ) in vec3 inNormal;
                                                        layout ( location = 2 ) in vec2 inTexCoord;

                                                        layout ( set = 0, binding = 0 ) uniform RenderPassUniforms {
                                                     		mat4 view;
                                                         	mat4 proj;
                                                        };

                                                        layout ( location = 0 ) out vec3 outPosition;

                                                        void main() {
                                                     		gl_Position = proj * view * vec4( inPosition, 1.0 );
                                                         	outPosition = inPosition;
                                                        }
                                                 	)"
                                             	),
                                                crimild::alloc< Shader >(
                                                    Shader::Stage::FRAGMENT,
                                                    R"(
                                                    	layout ( location = 0 ) in vec3 inPosition;

                                                 		layout ( set = 1, binding = 0 ) uniform sampler2D uHDRMap;

                                                        layout ( location = 0 ) out vec4 outColor;

                                                     	const vec2 invAtan = vec2( 0.1591, 0.3183 );

                                                     	vec2 sampleSphericalMap( vec3 v )
                                                    	{
                                                        	vec2 uv = vec2( atan( v.z, v.x ), asin( v.y ) );
                                                         	uv *= invAtan;
                                                         	uv += 0.5;
                                                        	uv.y = 1.0 - uv.y; // because of vulkan
                                                         	return uv;
                                                     	}

                                                        void main() {
                                                        	vec2 uv = sampleSphericalMap( normalize( inPosition ) );
                                                        	vec3 color = texture( uHDRMap, uv ).rgb;
                                                    		outColor = vec4( color, 1.0 );
                                                    	}
                                                 	)"
                                             	),
                                        	}
                                        );
                                        program->vertexLayouts = { VertexP3N3TC2::getLayout() };
                                        program->descriptorSetLayouts = {
                                            [] {
                                                auto layout = crimild::alloc< DescriptorSetLayout >();
                                                layout->bindings = {
                                                    {
                                                        .descriptorType = DescriptorType::UNIFORM_BUFFER,
                                                        .stage = Shader::Stage::VERTEX,
                                                    },
                                                };
                                                return layout;
                                            }(),
                                            [] {
                                                auto layout = crimild::alloc< DescriptorSetLayout >();
                                                layout->bindings = {
                                                    {
                                                        .descriptorType = DescriptorType::TEXTURE,
                                                        .stage = Shader::Stage::FRAGMENT,
                                                    },
                                                };
                                                return layout;
                                            }(),
                                            [] {
                                                auto layout = crimild::alloc< DescriptorSetLayout >();
                                                layout->bindings = {
                                                    {
                                                        .descriptorType = DescriptorType::UNIFORM_BUFFER,
                                                        .stage = Shader::Stage::VERTEX,
                                                    },
                                                };
                                                return layout;
                                            }(),
                                        };
                                        return program;
                                    }();
                                    return pipeline;
                                }()
                            );
                            return material;
                        }() );
                    return geometry;
                }() );

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
            auto withTonemapping = []( auto enabled, auto cmp ) {
                return enabled ? tonemapping( cmp, 1.0 ) : cmp;
            };
            return present( withTonemapping( false, renderSceneHDR( m_scene ) ) );
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

    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "PBR: IBL", crimild::alloc< Settings >( argc, argv ) );

    SharedPointer< ImageManager > imageManager = crimild::alloc< crimild::stb::ImageManager >();

    sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );

    return sim->run();
}
