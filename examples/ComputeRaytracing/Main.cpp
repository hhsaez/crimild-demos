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
                            settings->set( "rendering.samples", UInt32( 1 ) );
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

                            auto resetSampling = [ settings ] {
                                settings->set( "rendering.samples", UInt32( 1 ) );
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

                    struct MaterialDesc {
                        alignas( 16 ) ColorRGB albedo = ColorRGB::Constants::WHITE;
                        alignas( 4 ) Real32 metallic = 0;
                        alignas( 4 ) Real32 roughness = 0;
                        alignas( 4 ) Real32 ambientOcclusion = 1;
                        alignas( 4 ) Real32 transmission = 0;
                        alignas( 4 ) Real32 indexOfRefraction = 0;
                        alignas( 16 ) ColorRGB emissive = ColorRGB::Constants::BLACK;
                    };

#define MAX_SPHERE_COUNT 500
#define MAX_MATERIAL_COUNT 500

                    struct SceneUniforms {
                        alignas( 16 ) SphereDesc spheres[ MAX_SPHERE_COUNT ];
                        alignas( 4 ) UInt32 sphereCount = 0;
                        alignas( 16 ) MaterialDesc materials[ MAX_MATERIAL_COUNT ];
                        alignas( 4 ) UInt32 materialCount = 0;
                    };

                    SceneUniforms uniforms;

                    auto createSphere = [ & ]( const Vector3f &center, Real32 radius, UInt32 materialID ) {
                        SphereDesc s;
                        s.invWorld = inverse( translation( center ) * scale( radius ) ).mat;
                        s.materialID = materialID;
                        uniforms.spheres[ uniforms.sphereCount ] = s;
                        return uniforms.sphereCount++;
                    };

                    auto createLambertianMaterial = [ & ]( ColorRGB albedo ) {
                        MaterialDesc m;
                        m.albedo = albedo;
                        uniforms.materials[ uniforms.materialCount ] = m;
                        return uniforms.materialCount++;
                    };

                    auto createMetalMaterial = [ & ]( ColorRGB albedo, float fuzz ) {
                        MaterialDesc m;
                        m.albedo = albedo;
                        m.metallic = 1.0f;
                        m.roughness = fuzz;
                        uniforms.materials[ uniforms.materialCount ] = m;
                        return uniforms.materialCount++;
                    };

                    auto createDielectricMaterial = [ & ]( float ir ) {
                        MaterialDesc m;
                        m.transmission = 1.0f;
                        m.indexOfRefraction = ir;
                        uniforms.materials[ uniforms.materialCount ] = m;
                        return uniforms.materialCount++;
                    };

                    int groundMaterial = createLambertianMaterial( ColorRGB { 0.5, 0.5, 0.5 } );
                    createSphere( Vector3f { 0, -1000, 0 }, 1000, groundMaterial );

                    int count = 11;

                    for ( int a = -count; a < count; a++ ) {
                        for ( int b = -count; b < count; b++ ) {
                            Real32 chooseMat = Random::generate< Real32 >();
                            Vector3f center {
                                Real( a + 0.9 * Random::generate< Real32 >() ),
                                Real( 0.2 ),
                                Real( b + 0.9 * Random::generate< Real32 >() ),
                            };

                            if ( length( center - Vector3 { 4, 0.2, 0 } ) > 0.9f ) {
                                if ( chooseMat < 0.8 ) {
                                    // diffuse
                                    auto albedo = ColorRGB {
                                        Random::generate< Real32 >() * Random::generate< Real32 >(),
                                        Random::generate< Real32 >() * Random::generate< Real32 >(),
                                        Random::generate< Real32 >() * Random::generate< Real32 >(),
                                    };
                                    int sphereMaterial = createLambertianMaterial( albedo );
                                    createSphere( center, 0.2, sphereMaterial );
                                } else if ( chooseMat < 0.95 ) {
                                    // metal
                                    auto albedo = ColorRGB {
                                        Random::generate< Real32 >( 0.5, 1.0 ),
                                        Random::generate< Real32 >( 0.5, 1.0 ),
                                        Random::generate< Real32 >( 0.5, 1.0 ),
                                    };
                                    float fuzz = Random::generate< Real32 >( 0, 0.5 );
                                    int sphereMaterial = createMetalMaterial( albedo, fuzz );
                                    createSphere( center, 0.2, sphereMaterial );
                                } else {
                                    // glass
                                    int sphereMaterial = createDielectricMaterial( 1.5 );
                                    createSphere( center, 0.2, sphereMaterial );
                                }
                            }
                        }
                    }

                    int material1 = createDielectricMaterial( 1.5 );
                    createSphere( Vector3 { 0, 1, 0 }, 1.0, material1 );

                    int material2 = createLambertianMaterial( ColorRGB { 0.4, 0.2, 0.1 } );
                    createSphere( Vector3 { -4, 1, 0 }, 1.0, material2 );

                    int material3 = createMetalMaterial( ColorRGB { 0.7, 0.6, 0.5 }, 0.0 );
                    createSphere( Vector3 { 4, 1, 0 }, 1.0, material3 );

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
                                Format::R8G8B8A8_UNORM,
                                descriptors ) ) ) );
            }() );
    }
};

CRIMILD_CREATE_SIMULATION( Example, "Compute: Raytracing" );
