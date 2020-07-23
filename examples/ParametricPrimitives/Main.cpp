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

using namespace crimild;
using namespace crimild::glfw;
using namespace crimild::vulkan;

class GeometryParams : public NodeComponent {
    CRIMILD_IMPLEMENT_RTTI( GeometryParams )
public:
    explicit GeometryParams( SharedPointer< Pipeline > const &pipeline ) : m_pipeline( pipeline ) { }
    virtual ~GeometryParams( void ) = default;

    Pipeline *getPipeline( void ) noexcept { return get_ptr( m_pipeline ); }

private:
    SharedPointer< Pipeline > m_pipeline;
};

class ExampleVulkanSystem : public GLFWVulkanSystem {
public:
    crimild::Bool start( void ) override
    {
        if ( !GLFWVulkanSystem::start() ) {
            return false;
        }

		m_frameGraph = crimild::alloc< FrameGraph >();

        auto pipelineBuilder = []( auto primitiveType ) {
            auto pipeline = crimild::alloc< Pipeline >();
            pipeline->primitiveType = primitiveType;
            pipeline->program = [&] {
                auto createShader = []( Shader::Stage stage, std::string path ) {
                    return crimild::alloc< Shader >(
                        stage,
                        FileSystem::getInstance().readFile(
                            FilePath {
                                .path = path,
                            }.getAbsolutePath()
                        )
                    );
                };

                auto program = crimild::alloc< ShaderProgram >(
                    Array< SharedPointer< Shader >> {
                        createShader(
                            Shader::Stage::VERTEX,
                            "assets/shaders/scene.vert.spv"
                        ),
                        createShader(
                            Shader::Stage::FRAGMENT,
                            "assets/shaders/scene.frag.spv"
                        ),
                        }
                );
                program->vertexLayouts = { VertexP3C3::getLayout() };
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
        };

        auto solidPipeline = pipelineBuilder( Primitive::Type::TRIANGLES );
        auto linesPipeline = pipelineBuilder( Primitive::Type::LINES );

        m_scene = [&] {
            auto scene = crimild::alloc< Group >();

            // Spheres
            scene->attachNode(
                [&] {
                    auto spheres = crimild::alloc< Group >();

                    auto sphere = [&]( const Vector3f &position, const Vector2f &divisions ) {
                        auto group = crimild::alloc< Group >();
                        group->attachNode(
                            [&] {
                                auto geometry = crimild::alloc< Geometry >();
                                geometry->attachPrimitive(
                                    crimild::alloc< ParametricSpherePrimitive >(
                                        ParametricSpherePrimitive::Params {
                                            .type = Primitive::Type::TRIANGLES,
                                            .layout = VertexP3C3::getLayout(),
                                            .radius = 1.0f,
                                            .colorMode = { .type = ParametricPrimitive::ColorMode::Type::POSITIONS },
                                            .divisions = divisions,
                                        }
                                    )
                                );
                                geometry->attachComponent< GeometryParams >( solidPipeline );
                                return geometry;
                            }()
                        );
                        group->attachNode(
                            [&] {
                                auto geometry = crimild::alloc< Geometry >();
                                geometry->attachPrimitive(
                                    crimild::alloc< ParametricSpherePrimitive >(
                                        ParametricSpherePrimitive::Params {
                                            .type = Primitive::Type::LINES,
                                            .layout = VertexP3C3::getLayout(),
                                            .radius = 1.1f,
                                            .divisions = divisions,
                                        }
                                    )
                                );
                                geometry->attachComponent< GeometryParams >( linesPipeline );
                                return geometry;
                            }()
                        );
                        group->local().setTranslate( position );
                        return group;
                    };

                    spheres->attachNode( sphere( Vector3f( -6.0f, 0.0f, 0.0f ), Vector2f( 40.0f, 40.0f ) ) );
                    spheres->attachNode( sphere( Vector3f( -3.0f, 0.0f, 0.0f ), Vector2f( 30.0f, 30.0f ) ) );
                    spheres->attachNode( sphere( Vector3f( 0.0f, 0.0f, 0.0f ), Vector2f( 20.0f, 20.0f ) ) );
                    spheres->attachNode( sphere( Vector3f( 3.0f, 0.0f, 0.0f ), Vector2f( 10.0f, 10.0f ) ) );
                    spheres->attachNode( sphere( Vector3f( 6.0f, 0.0f, 0.0f ), Vector2f( 5.0f, 5.0f ) ) );

                    return spheres;
                }()
            );

            // Cones
            scene->attachNode(
                [&] {
                    auto cones = crimild::alloc< Group >();

                    auto cone = [&]( const Vector3f &position, const Vector2f &divisions ) {
                        auto group = crimild::alloc< Group >();
                        group->attachNode(
                            [&] {
                                auto geometry = crimild::alloc< Geometry >();
                                geometry->attachPrimitive(
                                    crimild::alloc< ConePrimitive >(
                                        ConePrimitive::Params {
                                            .type = Primitive::Type::TRIANGLES,
                                            .layout = VertexP3C3::getLayout(),
                                            .colorMode = { .type = ParametricPrimitive::ColorMode::Type::POSITIONS },
                                            .divisions = divisions,
                                        }
                                    )
                                );
                                geometry->attachComponent< GeometryParams >( solidPipeline );
                                return geometry;
                            }()
                        );
                        group->attachNode(
                            [&] {
                                auto geometry = crimild::alloc< Geometry >();
                                geometry->attachPrimitive(
                                    crimild::alloc< ConePrimitive >(
                                        ConePrimitive::Params {
                                            .type = Primitive::Type::LINES,
                                            .layout = VertexP3C3::getLayout(),
                                            .radius = 0.6f,
                                            .height = 1.1f,
                                            .divisions = divisions,
                                        }
                                    )
                                );
                                geometry->attachComponent< GeometryParams >( linesPipeline );
                                return geometry;
                            }()
                        );
                        group->local().setTranslate( position );
                        return group;
                    };

                    cones->attachNode( cone( Vector3f( -6.0f, 0.0f, 0.0f ), Vector2f( 40.0f, 40.0f ) ) );
                    cones->attachNode( cone( Vector3f( -3.0f, 0.0f, 0.0f ), Vector2f( 30.0f, 30.0f ) ) );
                    cones->attachNode( cone( Vector3f( 0.0f, 0.0f, 0.0f ), Vector2f( 20.0f, 20.0f ) ) );
                    cones->attachNode( cone( Vector3f( 3.0f, 0.0f, 0.0f ), Vector2f( 10.0f, 10.0f ) ) );
                    cones->attachNode( cone( Vector3f( 6.0f, 0.0f, 0.0f ), Vector2f( 5.0f, 5.0f ) ) );

                    cones->local().setTranslate( Vector3f( 0.0f, 3.0f, 0.0f ) );

                    return cones;
                }()
            );

            // KleinBottles
            scene->attachNode(
                [&] {
                    auto kleinBottles = crimild::alloc< Group >();

                    auto kleinBottle = [&]( const Vector3f &position, const Vector2f &divisions ) {
                        auto group = crimild::alloc< Group >();
                        group->attachNode(
                            [&] {
                                auto geometry = crimild::alloc< Geometry >();
                                geometry->attachPrimitive(
                                    crimild::alloc< KleinBottlePrimitive >(
                                        KleinBottlePrimitive::Params {
                                            .type = Primitive::Type::TRIANGLES,
                                            .layout = VertexP3C3::getLayout(),
                                            .colorMode = { .type = ParametricPrimitive::ColorMode::Type::POSITIONS },
                                            .divisions = divisions,
                                        }
                                    )
                                );
                                geometry->attachComponent< GeometryParams >( solidPipeline );
                                return geometry;
                            }()
                        );
                        group->attachNode(
                            [&] {
                                auto geometry = crimild::alloc< Geometry >();
                                geometry->attachPrimitive(
                                    crimild::alloc< KleinBottlePrimitive >(
                                        KleinBottlePrimitive::Params {
                                            .type = Primitive::Type::LINES,
                                            .layout = VertexP3C3::getLayout(),
                                            .scale = 1.1f,
                                            .divisions = divisions,
                                        }
                                    )
                                );
                                geometry->attachComponent< GeometryParams >( linesPipeline );
                                return geometry;
                            }()
                        );
                        group->local().setTranslate( position );
                        return group;
                    };

                    kleinBottles->attachNode( kleinBottle( Vector3f( -6.0f, 0.0f, 0.0f ), Vector2f( 40.0f, 40.0f ) ) );
                    kleinBottles->attachNode( kleinBottle( Vector3f( -3.0f, 0.0f, 0.0f ), Vector2f( 30.0f, 30.0f ) ) );
                    kleinBottles->attachNode( kleinBottle( Vector3f( 0.0f, 0.0f, 0.0f ), Vector2f( 20.0f, 20.0f ) ) );
                    kleinBottles->attachNode( kleinBottle( Vector3f( 3.0f, 0.0f, 0.0f ), Vector2f( 10.0f, 10.0f ) ) );
                    kleinBottles->attachNode( kleinBottle( Vector3f( 6.0f, 0.0f, 0.0f ), Vector2f( 5.0f, 5.0f ) ) );

                    kleinBottles->local().setTranslate( Vector3f( 0.0f, -3.0f, 0.0f ) );

                    return kleinBottles;
                }()
            );

            // Treefoil Knot
            scene->attachNode(
                [&] {
                    auto trefoilKnots = crimild::alloc< Group >();

                    auto trefoilKnot = [&]( const Vector3f &position, const Vector2f &divisions ) {
                        auto group = crimild::alloc< Group >();
                        group->attachNode(
                            [&] {
                                auto geometry = crimild::alloc< Geometry >();
                                geometry->attachPrimitive(
                                    crimild::alloc< TrefoilKnotPrimitive >(
                                        TrefoilKnotPrimitive::Params {
                                            .type = Primitive::Type::TRIANGLES,
                                            .layout = VertexP3C3::getLayout(),
                                            .colorMode = { .type = ParametricPrimitive::ColorMode::Type::POSITIONS },
                                            .divisions = divisions,
                                        }
                                    )
                                );
                                geometry->attachComponent< GeometryParams >( solidPipeline );
                                return geometry;
                            }()
                        );
                        group->attachNode(
                            [&] {
                                auto geometry = crimild::alloc< Geometry >();
                                geometry->attachPrimitive(
                                    crimild::alloc< TrefoilKnotPrimitive >(
                                        TrefoilKnotPrimitive::Params {
                                            .type = Primitive::Type::LINES,
                                            .layout = VertexP3C3::getLayout(),
                                            .scale = 1.1f,
                                            .divisions = divisions,
                                        }
                                    )
                                );
                                geometry->attachComponent< GeometryParams >( linesPipeline );
                                return geometry;
                            }()
                        );
                        group->local().setTranslate( position );
                        return group;
                    };

                    trefoilKnots->attachNode( trefoilKnot( Vector3f( -6.0f, 0.0f, 0.0f ), Vector2f( 40.0f, 40.0f ) ) );
                    trefoilKnots->attachNode( trefoilKnot( Vector3f( -3.0f, 0.0f, 0.0f ), Vector2f( 30.0f, 30.0f ) ) );
                    trefoilKnots->attachNode( trefoilKnot( Vector3f( 0.0f, 0.0f, 0.0f ), Vector2f( 20.0f, 20.0f ) ) );
                    trefoilKnots->attachNode( trefoilKnot( Vector3f( 3.0f, 0.0f, 0.0f ), Vector2f( 10.0f, 10.0f ) ) );
                    trefoilKnots->attachNode( trefoilKnot( Vector3f( 6.0f, 0.0f, 0.0f ), Vector2f( 5.0f, 5.0f ) ) );

                    trefoilKnots->local().setTranslate( Vector3f( 0.0f, -6.0f, 0.0f ) );

                    return trefoilKnots;
                }()
            );

            // MobiusStrips
            scene->attachNode(
                [&] {
                    auto mobiusStrips = crimild::alloc< Group >();

                    auto mobiusStrip = [&]( const Vector3f &position, const Vector2f &divisions ) {
                        auto group = crimild::alloc< Group >();
                        group->attachNode(
                            [&] {
                                auto geometry = crimild::alloc< Geometry >();
                                geometry->attachPrimitive(
                                    crimild::alloc< MobiusStripPrimitive >(
                                        MobiusStripPrimitive::Params {
                                            .type = Primitive::Type::TRIANGLES,
                                            .layout = VertexP3C3::getLayout(),
                                            .colorMode = { .type = ParametricPrimitive::ColorMode::Type::POSITIONS },
                                            .divisions = divisions,
                                        }
                                    )
                                );
                                geometry->attachComponent< GeometryParams >( solidPipeline );
                                return geometry;
                            }()
                        );
                        group->attachNode(
                            [&] {
                                auto geometry = crimild::alloc< Geometry >();
                                geometry->attachPrimitive(
                                    crimild::alloc< MobiusStripPrimitive >(
                                        MobiusStripPrimitive::Params {
                                            .type = Primitive::Type::LINES,
                                            .layout = VertexP3C3::getLayout(),
                                            .scale = 1.1f,
                                            .divisions = divisions,
                                        }
                                    )
                                );
                                geometry->attachComponent< GeometryParams >( linesPipeline );
                                return geometry;
                            }()
                        );
                        group->local().setTranslate( position );
                        return group;
                    };

                    mobiusStrips->attachNode( mobiusStrip( Vector3f( -6.0f, 0.0f, 0.0f ), Vector2f( 40.0f, 40.0f ) ) );
                    mobiusStrips->attachNode( mobiusStrip( Vector3f( -3.0f, 0.0f, 0.0f ), Vector2f( 30.0f, 30.0f ) ) );
                    mobiusStrips->attachNode( mobiusStrip( Vector3f( 0.0f, 0.0f, 0.0f ), Vector2f( 20.0f, 20.0f ) ) );
                    mobiusStrips->attachNode( mobiusStrip( Vector3f( 3.0f, 0.0f, 0.0f ), Vector2f( 10.0f, 10.0f ) ) );
                    mobiusStrips->attachNode( mobiusStrip( Vector3f( 6.0f, 0.0f, 0.0f ), Vector2f( 5.0f, 5.0f ) ) );

                    mobiusStrips->local().setTranslate( Vector3f( 0.0f, -9.0f, 0.0f ) );

                    return mobiusStrips;
                }()
            );

            // Toruss
            scene->attachNode(
                [&] {
                    auto toruses = crimild::alloc< Group >();

                    auto torus = [&]( const Vector3f &position, const Vector2f &divisions ) {
                        auto group = crimild::alloc< Group >();
                        group->attachNode(
                            [&] {
                                auto geometry = crimild::alloc< Geometry >();
                                geometry->attachPrimitive(
                                    crimild::alloc< TorusPrimitive >(
                                        TorusPrimitive::Params {
                                            .type = Primitive::Type::TRIANGLES,
                                            .layout = VertexP3C3::getLayout(),
                                            .colorMode = { .type = ParametricPrimitive::ColorMode::Type::POSITIONS },
                                            .divisions = divisions,
                                        }
                                    )
                                );
                                geometry->attachComponent< GeometryParams >( solidPipeline );
                                return geometry;
                            }()
                        );
                        group->attachNode(
                            [&] {
                                auto geometry = crimild::alloc< Geometry >();
                                geometry->attachPrimitive(
                                    crimild::alloc< TorusPrimitive >(
                                        TorusPrimitive::Params {
                                            .type = Primitive::Type::LINES,
                                            .layout = VertexP3C3::getLayout(),
                                            .minorRadius = 0.2f,
                                            .majorRadius = 0.6f,
                                            .divisions = divisions,
                                        }
                                    )
                                );
                                geometry->attachComponent< GeometryParams >( linesPipeline );
                                return geometry;
                            }()
                        );
                        group->local().setTranslate( position );
                        return group;
                    };

                    toruses->attachNode( torus( Vector3f( -6.0f, 0.0f, 0.0f ), Vector2f( 40.0f, 40.0f ) ) );
                    toruses->attachNode( torus( Vector3f( -3.0f, 0.0f, 0.0f ), Vector2f( 30.0f, 30.0f ) ) );
                    toruses->attachNode( torus( Vector3f( 0.0f, 0.0f, 0.0f ), Vector2f( 20.0f, 20.0f ) ) );
                    toruses->attachNode( torus( Vector3f( 3.0f, 0.0f, 0.0f ), Vector2f( 10.0f, 10.0f ) ) );
                    toruses->attachNode( torus( Vector3f( 6.0f, 0.0f, 0.0f ), Vector2f( 5.0f, 5.0f ) ) );

                    toruses->local().setTranslate( Vector3f( 0.0f, -12.0f, 0.0f ) );

                    return toruses;
                }()
            );

            scene->attachNode([] {
                auto camera = crimild::alloc< Camera >();
                camera->local().setTranslate( 0.0f, -4.5f, 24.0f );
                Camera::setMainCamera( camera );
                return camera;
            }());
            return scene;
        }();

		m_renderPass = [&] {
            auto renderPass = crimild::alloc< RenderPass >();
            renderPass->attachments = {
                [&] {
                    auto att = crimild::alloc< Attachment >();
                    att->format = Format::COLOR_SWAPCHAIN_OPTIMAL;
                    return att;
                }(),
            };
            renderPass->setDescriptors(
                [&] {
                    auto descriptorSet = crimild::alloc< DescriptorSet >();
                    descriptorSet->descriptors = {
                        Descriptor {
                            .descriptorType = DescriptorType::UNIFORM_BUFFER,
                            .obj = [&] {
                                FetchCameras fetch;
                                m_scene->perform( fetch );
                                auto camera = fetch.anyCamera();
                                return crimild::alloc< CameraViewProjectionUniform >( camera );
                            }(),
                        },
                    };
                    return descriptorSet;
                }()
            );
            renderPass->commands = [&] {
                auto commandBuffer = crimild::alloc< CommandBuffer >();
                m_scene->perform(
                    ApplyToGeometries(
                        [&]( Geometry *g ) {
                            auto pipeline = g->getComponent< GeometryParams >()->getPipeline();
                            commandBuffer->bindGraphicsPipeline( pipeline );
                            commandBuffer->bindDescriptorSet( renderPass->getDescriptors() );
                            commandBuffer->bindDescriptorSet( g->getDescriptors() );
                            auto p = g->anyPrimitive();
                            auto vertices = p->getVertexData()[ 0 ];
                            auto indices = p->getIndices();
                            commandBuffer->bindVertexBuffer( get_ptr( vertices ) );
                            commandBuffer->bindIndexBuffer( indices );
                            commandBuffer->drawIndexed( indices->getIndexCount() );
						}
					)
				);
				return commandBuffer;
			}();

			return renderPass;
		}();

		m_master = [&] {
            auto master = crimild::alloc< PresentationMaster >();
            master->colorAttachment = m_renderPass->attachments[ 0 ];
			return master;
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
        m_renderPass = nullptr;
        m_master = nullptr;
        m_frameGraph = nullptr;

        GLFWVulkanSystem::stop();
    }

private:
    SharedPointer< Node > m_scene;
    SharedPointer< FrameGraph > m_frameGraph;
	SharedPointer< RenderPass > m_renderPass;
	SharedPointer< PresentationMaster > m_master;
};

int main( int argc, char **argv )
{
    crimild::init();
    crimild::vulkan::init();

    Log::setLevel( Log::Level::LOG_LEVEL_ALL );

    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "Parametric Primitives", crimild::alloc< Settings >( argc, argv ) );
    sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );
    return sim->run();
}