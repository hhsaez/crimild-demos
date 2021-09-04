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
#include <Crimild_ImGUI.hpp>

using namespace crimild;

namespace crimild {

    struct RenderSettings {
        alignas( 4 ) UInt32 sampleCount = 0;
        alignas( 4 ) UInt32 maxSamples = 500;
        alignas( 4 ) UInt32 maxBounces = 10;
        alignas( 4 ) Real32 aperture = 0.1;
        alignas( 4 ) Real32 focusDist = 10.0;

        friend std::ostream &operator<<( std::ostream &out, RenderSettings &settings ) noexcept
        {
            out << settings.sampleCount << " "
                << settings.maxSamples << " "
                << settings.maxBounces << " "
                << settings.aperture << " "
                << settings.focusDist << " ";
            return out;
        }

        friend std::istream &operator>>( std::istream &in, RenderSettings &settings ) noexcept
        {
            in >> settings.sampleCount >> std::skipws
                >> settings.maxSamples >> std::skipws
                >> settings.maxBounces >> std::skipws
                >> settings.aperture >> std::skipws
                >> settings.focusDist >> std::skipws;
            return in;
        }
    };

}

#include "rt.frag"

class Example : public Simulation {
public:
    void onStarted( void ) noexcept override
    {
        auto settings = getSettings();
        const float resolutionScale = 1.0f;
        const int width = resolutionScale * settings->get< Int32 >( "video.width", 1024 );
        const int height = resolutionScale * settings->get< Int32 >( "video.height", 768 );

        setScene(
            [ width, height ] {
                auto scene = crimild::alloc< Group >();

                auto sphere = [ & ]( const auto &center, Real radius, auto material ) -> SharedPointer< Node > {
                    auto geometry = crimild::alloc< Geometry >();
                    geometry->attachPrimitive( crimild::alloc< Primitive >( Primitive::Type::SPHERE ) );
                    geometry->setLocal( translation( vector3( center ) ) * scale( radius ) );
                    geometry->attachComponent< MaterialComponent >( material );
                    return geometry;
                };

                auto metallic = []( const auto &albedo, auto roughness ) -> SharedPointer< Material > {
                    auto material = crimild::alloc< materials::PrincipledBSDF >();
                    material->setAlbedo( albedo );
                    material->setMetallic( 1 );
                    material->setRoughness( roughness );
                    return material;
                };

                auto lambertian = []( const auto &albedo ) -> SharedPointer< Material > {
                    auto material = crimild::alloc< materials::PrincipledBSDF >();
                    material->setAlbedo( albedo );
                    return material;
                };

                auto emissive = []( const auto &color ) -> SharedPointer< Material > {
                    auto material = crimild::alloc< materials::PrincipledBSDF >();
                    material->setEmissive( color );
                    return material;
                };

                auto dielectric = []( auto ior ) -> SharedPointer< Material > {
                    auto material = crimild::alloc< materials::PrincipledBSDF >();
                    material->setTransmission( 1 );
                    material->setIndexOfRefraction( ior );
                    return material;
                };

                Array< SharedPointer< Node > > spheres;

                // Ground
                spheres.add(
                    sphere(
                        Point3 { 0, -1000, 0 },
                        1000,
                        lambertian( ColorRGB { 0.5, 0.5, 0.5 } ) ) );

                for ( auto a = -11; a < 11; a++ ) {
                    for ( auto b = -11; b < 11; b++ ) {
                        auto mat = Random::generate< Real >();
                        const auto center = Point3 {
                            a + 0.9f * Random::generate< Real >(),
                            0.2,
                            b + 0.9f * Random::generate< Real >(),
                        };

                        if ( length( center - Point3 { 4, 0.2, 0 } ) > 0.9f ) {
                            if ( mat < 0.7f ) {
                                // diffuse
                                const auto albedo = ColorRGB {
                                    Random::generate< Real >() * Random::generate< Real >(),
                                    Random::generate< Real >() * Random::generate< Real >(),
                                    Random::generate< Real >() * Random::generate< Real >(),
                                };
                                auto s = sphere( center, 0.2, lambertian( albedo ) );
                                spheres.add( s );
                                if ( mat < 0.3f ) {
                                    s->attachComponent< LambdaComponent >(
                                        [ center, start = Random::generate< Real >( 0, numbers::TWO_PI ) ]( auto node, auto c ) {
                                            node->setLocal(
                                                translation( vector3( center + Vector3 { 0, 0.2f * Numericf::remapSin( 0, 1, start + c.getCurrentTime() ), 0 } ) ) * scale( 0.2 ) );
                                        } );
                                }
                            } else if ( mat < 0.8f ) {
                                // emissive
                                const auto albedo = ColorRGB {
                                    1.0f + 4.0f * Random::generate< Real >(),
                                    1.0f + 4.0f * Random::generate< Real >(),
                                    1.0f + 4.0f * Random::generate< Real >(),
                                };
                                spheres.add( sphere( center, 0.2, emissive( albedo ) ) );
                            } else if ( mat < 0.95f ) {
                                // metal
                                const auto albedo = ColorRGB {
                                    Random::generate< Real >( 0.5f, 1.0f ),
                                    Random::generate< Real >( 0.5f, 1.0f ),
                                    Random::generate< Real >( 0.5f, 1.0f ),
                                };
                                const auto roughness = Random::generate( 0.0f, 0.5f );
                                spheres.add( sphere( center, 0.2, metallic( albedo, roughness ) ) );
                            } else {
                                // glass
                                spheres.add( sphere( center, 0.2, dielectric( 1.5f ) ) );
                            }
                        }
                    }
                }

                spheres.add( sphere( Point3 { 0, 1, 0 }, 1.0, dielectric( 1.5f ) ) );
                spheres.add( sphere( Point3 { -4, 1, 0 }, 1.0, lambertian( ColorRGB { 0.4, 0.2, 0.1 } ) ) );
                spheres.add( sphere( Point3 { 4, 1, 0 }, 1.0f, metallic( ColorRGB { 0.7, 0.6, 0.5 }, 0.0 ) ) );

                scene->attachNode( framegraph::utils::optimize( spheres ) );

                scene->attachNode(
                    [ width, height ] {
                        auto camera = crimild::alloc< Camera >( 20.0f, Real32( width ) / Real32( height ), 0.001f, 1024.0f );
                        camera->setLocal(
                            lookAt(
                                Point3 { 12, 2, 3 },
                                Point3 { 0, 0, 0 },
                                Vector3::Constants::UP ) );
                        camera->attachComponent< FreeLookCameraComponent >();
                        return camera;
                    }() );

                scene->perform( UpdateWorldState() );
                scene->perform( StartComponents() );

                return scene;
            }() );

        auto withTonemapping = []( auto op ) -> SharedPointer< FrameGraphOperation > {
            auto settings = Simulation::getInstance()->getSettings();
            auto enabled = settings->hasKey( "video.exposure" );

            using namespace crimild::framegraph;
            return enabled ? tonemapping( useResource( op ) ) : op;
        };

        registerMessageHandler< messaging::KeyReleased >(
            [ & ]( messaging::KeyReleased const &msg ) {
                if ( msg.key == CRIMILD_INPUT_KEY_ESCAPE ) {
                    getSettings()->set( "ui.show", !getSettings()->get< Bool >( "ui.show", true ) );
                    Camera::getMainCamera()->getComponent< FreeLookCameraComponent >()->setEnabled( !getSettings()->get< Bool >( "ui.show", true ) );
                }
            } );

        imgui::ImGUISystem::getInstance()->setFrameCallback(
            [ & ] {
                if ( !getSettings()->get< Bool >( "ui.show", true ) ) {
                    return;
                }

                auto renderSettings = getSettings()->get< RenderSettings >( "render.settings", RenderSettings {} );

                {
                    auto s = Simulation::getInstance()->getSimulationClock().getDeltaTime();
                    ImGui::Begin( "Stats" );
                    ImGui::Text( "Frame Time: %.2f ms", 1000.0f * s );
                    ImGui::Text( "FPS: %d", s > 0 ? int( 1.0 / s ) : 0 );
                    ImGui::Text( "Samples: %d/%d", renderSettings.sampleCount, renderSettings.maxSamples );
                    ImGui::End();
                }

                {
                    ImGui::Begin( "Render Settings" );

                    {
                        // Use AlignTextToFramePadding() to align text baseline to the baseline of framed elements (otherwise a Text+SameLine+Button sequence will have the text a little too high by default)
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text( "Max Samples: " );
                        ImGui::SameLine();

                        // Arrow buttons with Repeater
                        float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
                        ImGui::PushButtonRepeat( true );
                        if ( ImGui::ArrowButton( "##left", ImGuiDir_Left ) ) {
                            renderSettings.maxSamples--;
                        }
                        ImGui::SameLine( 0.0f, spacing );
                        if ( ImGui::ArrowButton( "##right", ImGuiDir_Right ) ) {
                            renderSettings.maxSamples++;
                        }
                        ImGui::PopButtonRepeat();
                        ImGui::SameLine();
                        ImGui::Text( "%d", UInt32( renderSettings.maxSamples ) );
                    }

                    {
                        ImGui::SliderFloat( "f", &renderSettings.focusDist, 0.0f, 10.0f, "Focus Distance = %.3f" );
                    }

                    {
                        ImGui::SliderFloat( "a", &renderSettings.aperture, 0.0f, 1.0f, "Aperture = %.3f" );
                    }

                    {
                        if ( ImGui::Button( "Reset" ) ) {
                            //settings->set( "rendering.samples", UInt32( 1 ) );
                            renderSettings.sampleCount = 0;
                        }
                    }

                    getSettings()->set( "render.settings", renderSettings );

                    ImGui::End();
                }
            } );

        auto descriptors = Array< Descriptor > {
            Descriptor {
                .descriptorType = DescriptorType::UNIFORM_BUFFER,
                .obj = crimild::alloc< CallbackUniformBuffer< RenderSettings > >(
                    [] {
                        auto settings = Simulation::getInstance()->getSettings();
                        auto renderSettings = settings->get< RenderSettings >( "render.settings", RenderSettings {} );
                        renderSettings.sampleCount = Numeric< UInt32 >::min( renderSettings.sampleCount + 1, renderSettings.maxSamples );
                        settings->set( "render.settings", renderSettings );
                        return renderSettings;
                    } ),
            },
            Descriptor {
                .descriptorType = DescriptorType::UNIFORM_BUFFER,
                .obj = [] {
                    struct Uniforms {
                        alignas( 4 ) UInt32 sampleCount;
                        alignas( 4 ) UInt32 maxSamples;
                        alignas( 4 ) UInt32 seed;
                        alignas( 4 ) Real32 tanHalfFOV;
                        alignas( 4 ) Real32 aspectRatio;
                        alignas( 4 ) Real32 aperture;
                        alignas( 4 ) Real32 focusDist;
                        alignas( 16 ) Matrix4 proj;
                        alignas( 16 ) Matrix4 view;
                    };

                    return crimild::alloc< CallbackUniformBuffer< Uniforms > >(
                        [] {
                            auto settings = Simulation::getInstance()->getSettings();

                            auto renderSettings = settings->get< RenderSettings >( "render.settings", RenderSettings {} );

                            auto resetSampling = [ & ] {
                                //settings->set( "rendering.samples", UInt32( 1 ) );
                                renderSettings.sampleCount = 0;
                            };

                            static auto proj = Matrix4::Constants::IDENTITY;
                            static auto view = Matrix4::Constants::IDENTITY;
                            static auto focusDist = 10.0f;

                            auto camera = Camera::getMainCamera();
                            if ( camera != nullptr ) {
                                const auto cameraProj = camera->getProjectionMatrix();
                                if ( cameraProj != proj ) {
                                    proj = cameraProj;
                                    resetSampling();
                                }

                                const auto &cameraView = camera->getViewMatrix();
                                if ( cameraView != view ) {
                                    view = cameraView;
                                    resetSampling();
                                }
                            }

                            if ( !settings->get< Bool >( "ui.show", true ) ) {
                                if ( Input::getInstance()->isMouseButtonDown( CRIMILD_INPUT_MOUSE_BUTTON_LEFT ) ) {
                                    resetSampling();
                                }
                            }

                            settings->set(
                                "rendering.samples",
                                Numeric< UInt32 >::min(
                                    settings->get< UInt32 >( "rendering.samples", 1 ) + 1,
                                    renderSettings.maxSamples ) );

                            return Uniforms {
                                .sampleCount = settings->get< UInt32 >( "rendering.samples", 1 ),
                                .maxSamples = renderSettings.maxSamples,
                                .seed = settings->get< UInt32 >( "rendering.samples", 1 ),
                                .tanHalfFOV = radians( 90 ), //camera->getFrustum().computeTanHalfFOV(),
                                .aspectRatio = 4.0 / 3.0,    //camera->computeAspect(),
                                .aperture = renderSettings.aperture,
                                .focusDist = renderSettings.focusDist,
                                .proj = proj,
                                .view = view,
                            };
                        } );
                }(),
            },
            Descriptor {
                .descriptorType = DescriptorType::UNIFORM_BUFFER,
                .obj = [] {
                    struct SphereDesc {
                        alignas( 16 ) Matrix4 invWorld;
                        alignas( 4 ) UInt32 materialID;
                    };

#define MAX_SPHERE_COUNT 500
#define MAX_MATERIAL_COUNT 500

                    struct SceneUniforms {
                        alignas( 16 ) SphereDesc spheres[ MAX_SPHERE_COUNT ];
                        alignas( 4 ) UInt32 sphereCount = 0;
                        alignas( 16 ) materials::PrincipledBSDF::Props materials[ MAX_MATERIAL_COUNT ];
                        alignas( 4 ) UInt32 materialCount = 0;
                    };

                    SceneUniforms uniforms;

                    Map< Material *, UInt32 > materialIds;

                    auto scene = Simulation::getInstance()->getScene();
                    if ( scene != nullptr ) {
                        scene->perform(
                            ApplyToGeometries(
                                [ & ]( Geometry *geometry ) {
                                    const auto material = static_cast< materials::PrincipledBSDF * >( geometry->getComponent< MaterialComponent >()->first() );
                                    if ( !materialIds.contains( material ) ) {
                                        const auto materialId = uniforms.materialCount++;
                                        uniforms.materials[ materialId ] = material->getProps();
                                        materialIds.insert( material, materialId );
                                    }

                                    geometry->forEachPrimitive(
                                        [ & ]( auto primitive ) {
                                            if ( primitive->getType() == Primitive::Type::SPHERE ) {
                                                uniforms.spheres[ uniforms.sphereCount++ ] = {
                                                    .invWorld = geometry->getWorld().invMat,
                                                    .materialID = materialIds[ material ],
                                                };
                                            }
                                        } );
                                } ) );
                    }

                    return crimild::alloc< UniformBuffer >( uniforms );
                }(),
            },
        };

        RenderSystem::getInstance()->setFrameGraph(
            [ & ] {
                using namespace crimild::framegraph;
                return present(
                    useResource(
                        withTonemapping(
                            computeImage(
                                Extent2D {
                                    .width = Real32( width ),
                                    .height = Real32( height ),
                                },
                                crimild::alloc< Shader >(
                                    Shader::Stage::COMPUTE,
                                    FRAG_SRC ),
                                Format::R32G32B32A32_SFLOAT,
                                descriptors ) ) ) );
            }() );
    }
};

CRIMILD_CREATE_SIMULATION( Example, "Compute: Raytracing" );
