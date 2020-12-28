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

namespace crimild {

    namespace compositions {

        Composition computeImage( UInt32 width, UInt32 height, SharedPointer< Shader > const &shader ) noexcept
        {
            Composition cmp;

            auto texture = cmp.create< Texture >();
            texture->imageView = [ & ] {
                auto imageView = crimild::alloc< ImageView >();
                imageView->image = [ & ] {
                    auto image = crimild::alloc< Image >();
                    image->format = Format::R8G8B8A8_UNORM;
                    image->extent = {
                        .width = Real32( width ),
                        .height = Real32( height ),
                    };
                    image->setMipLevels( 1 );
                    image->setName( "Compute Image" );
                    return image;
                }();
                return imageView;
            }();
            texture->sampler = crimild::alloc< Sampler >();

            auto pipeline = [ & ] {
                auto pipeline = cmp.create< ComputePipeline >();
                pipeline->setName( "Compute Pipeline" );
                pipeline->setProgram(
                    [ & ] {
                        auto program = crimild::alloc< ShaderProgram >(
                            Array< SharedPointer< Shader > > {
                                shader,
                            } );
                        program->descriptorSetLayouts = {
                            [] {
                                auto layout = crimild::alloc< DescriptorSetLayout >();
                                layout->bindings = {
                                    {
                                        .descriptorType = DescriptorType::STORAGE_IMAGE,
                                        .stage = Shader::Stage::COMPUTE,
                                    },
                                    {
                                        .descriptorType = DescriptorType::UNIFORM_BUFFER,
                                        .stage = Shader::Stage::COMPUTE,
                                    },
                                };
                                return layout;
                            }(),
                        };
                        return program;
                    }() );
                return pipeline;
            }();

            auto descriptors = [ & ] {
                auto ds = cmp.create< DescriptorSet >();
                ds->descriptors = {
                    Descriptor {
                        .descriptorType = DescriptorType::STORAGE_IMAGE,
                        .obj = crimild::retain( texture ),
                    },
                    Descriptor {
                        .descriptorType = DescriptorType::UNIFORM_BUFFER,
                        .obj = [] {
                            struct Uniforms {
                                alignas( 4 ) UInt32 sampleCount;
                                alignas( 4 ) UInt32 seed;
                                alignas( 16 ) Matrix4f view;
                            };

                            return crimild::alloc< CallbackUniformBuffer< Uniforms > >(
                                [] {
                                    static UInt32 sampleCount = 1;
                                    static auto view = Matrix4f::IDENTITY;

                                    auto camera = Camera::getMainCamera();
                                    if ( camera != nullptr ) {
                                        const auto cameraView = camera->getWorld().computeModelMatrix();
                                        if ( cameraView != view ) {
                                            view = cameraView;
                                            sampleCount = 1;
                                        }
                                    }

                                    return Uniforms {
                                        .sampleCount = sampleCount++,
                                        .seed = sampleCount,
                                        .view = view,
                                    };
                                } );
                        }(),
                    },
                };
                return ds;
            }();

            auto computePass = cmp.create< ComputePass >();
            computePass->commands = [ & ] {
                auto commands = crimild::alloc< CommandBuffer >();
                commands->begin( CommandBuffer::Usage::SIMULTANEOUS_USE );
                commands->bindComputePipeline( pipeline );
                commands->bindDescriptorSet( descriptors );
                commands->dispatch(
                    DispatchWorkgroup {
                        .x = width / DispatchWorkgroup::DEFAULT_WORGROUP_SIZE,
                        .y = height / DispatchWorkgroup::DEFAULT_WORGROUP_SIZE,
                        .z = 1 } );
                commands->end();
                return commands;
            }();
            // computePass->setConditional( true );

            cmp.setOutput( nullptr );
            cmp.setOutputTexture( texture );
            return cmp;
        }
    }
}

class Example : public Simulation {
public:
    void onStarted( void ) noexcept override
    {
        auto settings = getSettings();
        const float resolutionScale = 0.25f;
        const int width = resolutionScale * settings->get< Int32 >( "video.width", 1024 );
        const int height = resolutionScale * settings->get< Int32 >( "video.height", 768 );

        setScene(
            [ width, height ] {
                auto scene = crimild::alloc< Group >();

                scene->attachNode(
                    [ width, height ] {
                        auto camera = crimild::alloc< Camera >( 30.0f, Real32( width ) / Real32( height ), 0.001f, 1024.0f );
                        camera->local().setTranslate( 0, 0, 3 );
                        Camera::setMainCamera( camera );
                        camera->attachComponent< FreeLookCameraComponent >();
                        return camera;
                    }() );

                scene->perform( StartComponents() );

                return scene;
            }() );

        setComposition(
            [ & ] {
                using namespace crimild::compositions;
                return present(
                    tonemapping(
                        computeImage(
                            width,
                            height,
                            crimild::alloc< Shader >(
                                Shader::Stage::COMPUTE,
                                R"(
                                layout( local_size_x = 32, local_size_y = 32 ) in;
                                layout( set = 0, binding = 0, rgba8 ) uniform image2D resultImage;

                                layout ( set = 0, binding = 1 ) uniform Uniforms {
                                    uint sampleCount;
                                    uint seedStart;
                                    mat4 view;
                                };

                                uint seed = 0;
                                int flat_idx = 0;

                                struct Sphere {
                                    vec3 center;
                                    float radius;
                                    int materialID;
                                };

                                Sphere createSphere( vec3 center, float radius, int materialID )
                                {
                                    Sphere s;
                                    s.center = center;
                                    s.radius = radius;
                                    s.materialID = materialID;
                                    return s;
                                }

                                #define MATERIAL_TYPE_LAMBERTIAN 0
                                #define MATERIAL_TYPE_METAL 1

                                struct Material {
                                    int type;
                                    vec3 albedo;
                                    float fuzz;
                                };

                                Material createLambertianMaterial( vec3 albedo )
                                {
                                    Material m;
                                    m.type = MATERIAL_TYPE_LAMBERTIAN;
                                    m.albedo = albedo;
                                    m.fuzz = 1.0;
                                    return m;
                                }

                                Material createMetalMaterial( vec3 albedo, float fuzz )
                                {
                                    Material m;
                                    m.type = MATERIAL_TYPE_METAL;
                                    m.albedo = albedo;
                                    m.fuzz = fuzz < 1.0 ? fuzz : 1.0;
                                    return m;
                                }

                                struct HitRecord {
                                    bool hasResult;
                                    float t;
                                    int materialID;
                                    vec3 point;
                                    vec3 normal;
                                    bool frontFace;
                                };

                                struct Ray {
                                    vec3 origin;
                                    vec3 direction;
                                };

                                struct Camera {
                                    vec2 viewport;
                                    float focalLength;
                                    vec3 origin;
                                    vec3 horizontal;
                                    vec3 vertical;
                                    vec3 lowerLeftCorner;
                                };

                                #define MATERIAL_COUNT 4
                                Material materials[ 4 ];

                                #define SPHERE_COUNT 4
                                Sphere spheres[ 4 ];

                                void encrypt_tea(inout uvec2 arg)
                                {
                                    uvec4 key = uvec4(0xa341316c, 0xc8013ea4, 0xad90777d, 0x7e95761e);
                                    uint v0 = arg[0], v1 = arg[1];
                                    uint sum = 0u;
                                    uint delta = 0x9e3779b9u;

                                    for(int i = 0; i < 32; i++) {
                                        sum += delta;
                                        v0 += ((v1 << 4) + key[0]) ^ (v1 + sum) ^ ((v1 >> 5) + key[1]);
                                        v1 += ((v0 << 4) + key[2]) ^ (v0 + sum) ^ ((v0 >> 5) + key[3]);
                                    }
                                    arg[0] = v0;
                                    arg[1] = v1;
                                }

                                float getRandom()
                                {
                                    uvec2 arg = uvec2( flat_idx, seed++);
                                    encrypt_tea(arg);
                                    vec2 r = fract(vec2(arg) / vec2(0xffffffffu));
                                    return r.x;
                                }

                                float getRandomRange( float min, float max )
                                {
                                    return min + getRandom() * ( max - min );
                                }

                                vec3 getRandomVec3()
                                {
                                    return vec3(
                                        getRandom(),
                                        getRandom(),
                                        getRandom()
                                    );
                                }

                                vec3 getRandomVec3Range( float min, float max )
                                {
                                    return vec3(
                                        getRandomRange( min, max ),
                                        getRandomRange( min, max ),
                                        getRandomRange( min, max )
                                    );
                                }

                                vec3 getRandomInUnitSphere()
                                {
                                    while ( true ) {
                                        vec3 p = getRandomVec3Range( -1.0, 1.0 );
                                        if ( dot( p, p ) < 1.0 ) {
                                            return p;
                                        }
                                    }
                                    return vec3( 0 );
                                }

                                vec3 getRandomUnitVector()
                                {
                                    return normalize( getRandomInUnitSphere() );
                                }

                                vec3 getRandomInHemisphere( vec3 N )
                                {
                                    vec3 inUnitSphere = getRandomInUnitSphere();
                                    if ( dot( inUnitSphere, N ) > 0.0 ) {
                                        return inUnitSphere;
                                    } else {
                                        return -inUnitSphere;
                                    }
                                }

                                HitRecord setFaceNormal( Ray ray, vec3 N, HitRecord rec )
                                {
                                    rec.frontFace = dot( ray.direction, N ) < 0;
                                    rec.normal = rec.frontFace ? N : -N;
                                    return rec;
                                }

                                vec3 rayAt( Ray ray, float t ) {
                                    return ray.origin + t * ray.direction;
                                }

                                struct Scattered {
                                    bool hasResult;
                                    Ray ray;
                                    vec3 attenuation;
                                };

                                bool isZero( vec3 v )
                                {
                                    float s = 0.00001;
                                    return abs( v.x ) < s && abs( v.y ) < s && abs( v.z ) < s;
                                }

                                Scattered scatter( Material material, Ray ray, HitRecord rec )
                                {
                                    Scattered scattered;
                                    scattered.hasResult = false;
                                    if ( material.type == MATERIAL_TYPE_LAMBERTIAN ) {
                                        vec3 scatterDirection = rec.normal + getRandomUnitVector();
                                        if ( isZero( scatterDirection ) ) {
                                            scatterDirection = rec.normal;
                                        }
                                        scattered.ray.origin = rec.point;
                                        scattered.ray.direction = scatterDirection;
                                        scattered.attenuation = material.albedo;
                                        scattered.hasResult = true;
                                    } else if ( material.type == MATERIAL_TYPE_METAL ) {
                                        vec3 reflected = reflect( normalize( ray.direction ), rec.normal );
                                        scattered.ray.origin = rec.point;
                                        scattered.ray.direction = reflected + material.fuzz * getRandomInUnitSphere();
                                        scattered.attenuation = material.albedo;
                                        scattered.hasResult = dot( scattered.ray.direction, rec.normal ) > 0.0;
                                    }
                                    return scattered;
                                }

                                HitRecord hitSphere( Sphere sphere, Ray ray, float tMin, float tMax ) {
                                    HitRecord rec;
                                    vec3 OC = ray.origin - sphere.center;
                                    float a = dot( ray.direction, ray.direction );
                                    float halfB = dot( OC, ray.direction );
                                    float c = dot( OC, OC ) - sphere.radius * sphere.radius;

                                    float discriminant = halfB * halfB - a * c;
                                    if ( discriminant < 0 ) {
                                        rec.hasResult = false;
                                        return rec;
                                    }

                                    float sqrtd = sqrt( discriminant );
                                    float root = ( -halfB - sqrtd ) / a;
                                    if ( root < tMin || root > tMax ) {
                                        root = ( -halfB + sqrtd ) / a;
                                        if ( root < tMin || root > tMax ) {
                                            rec.hasResult = false;
                                            return rec;
                                        }
                                    }

                                    rec.hasResult = true;
                                    rec.t = root;
                                    rec.materialID = sphere.materialID;
                                    rec.point = rayAt( ray, root );
                                    vec3 normal = ( rec.point - sphere.center ) / sphere.radius;
                                    return setFaceNormal( ray, normal, rec );
                                }

                                HitRecord hitScene( Ray ray, float tMin, float tMax )
                                {
                                    HitRecord hit;
                                    hit.t = tMax;
                                    for ( int i = 0; i < SPHERE_COUNT; i++ ) {
                                        HitRecord candidate = hitSphere( spheres[ i ], ray, tMin, hit.t );
                                        if ( candidate.hasResult ) {
                                            hit = candidate;
                                        }    
                                    }
                                    return hit;
                                }

                                Camera createCamera( float aspectRatio )
                                {
                                    Camera camera;
                                    camera.viewport = vec2( 2.0 * aspectRatio, 2.0 );
                                    camera.focalLength = 1.0;
                                    camera.origin = ( view * vec4( 0, 0, 0, 1 ) ).xyz;
                                    camera.horizontal = normalize( view * vec4( 1, 0, 0, 0 ) ).xyz;
                                    camera.vertical = normalize( view * vec4( 0, 1, 0, 0 ) ).xyz;
                                    vec3 forward = normalize( view * vec4( 0, 0, -1, 0 ) ).xyz;
                                    camera.lowerLeftCorner = camera.origin - camera.horizontal / 2.0 - camera.vertical / 2.0 + camera.focalLength * forward;
                                    return camera;
                                }

                                Ray getCameraRay( Camera camera, float u, float v )
                                {
                                    Ray ray;
                                    ray.origin = camera.origin;
                                    ray.direction = camera.lowerLeftCorner + u * camera.horizontal + v * camera.vertical - camera.origin;
                                    return ray;
                                }

                                vec3 rayColor( Ray ray ) {
                                    int maxDepth = 30;

                                    float multiplier = 1.0;
                                    float tMin = 0.001;
                                    float tMax = 9999.9;
                                    vec3 attenuation = vec3( 1.0 );

                                    vec3 color = vec3( 1.0 );

                                    int depth = 0;
                                    while ( true ) {
                                        if ( depth >= maxDepth ) {
                                            return color;
                                        }

                                        HitRecord hit = hitScene( ray, tMin, tMax );
                                        if ( !hit.hasResult ) {
                                            // no hit. use background color
                                            vec3 D = normalize( ray.direction );
                                            float t = 0.5 * ( D.y + 1.0 );
                                            vec3 backgroundColor = ( 1.0 - t ) * vec3( 1.0, 1.0, 1.0 ) + t * vec3( 0.5, 0.7, 1.0 );
                                            color *= backgroundColor;
                                            return color;
                                        } else {
                                            Scattered scattered = scatter( materials[ hit.materialID ], ray, hit );
                                            if ( scattered.hasResult ) {
                                                color *= scattered.attenuation;
                                                ray = scattered.ray;
                                                ++depth;
                                            } else {
                                                return color;
                                            }                                            
                                        }
                                    }

                                    // never happens
                                    return vec3( 0 );
                                }

                                vec3 gammaCorrection( vec3 color, int samplesPerPixel )
                                {
                                    float scale = 1.0 / float( samplesPerPixel );
                                    return sqrt( scale * color );
                                }

                                void main() {
                                    seed = seedStart;

                                    flat_idx = int(dot(gl_GlobalInvocationID.xy, vec2(1, 4096)));

                                    vec2 size = imageSize( resultImage );
                                    float aspectRatio = size.x / size.y;

                                    if ( gl_GlobalInvocationID.x >= size.x || gl_GlobalInvocationID.y >= size.y ) {
                                        return;
                                    }

                                    Camera camera = createCamera( aspectRatio );

                                    materials[ 0 ] = createLambertianMaterial( vec3( 0.8, 0.8, 0.0 ) );
                                    materials[ 1 ] = createLambertianMaterial( vec3( 0.7, 0.3, 0.3 ) );
                                    materials[ 2 ] = createMetalMaterial( vec3( 0.8, 0.8, 0.8 ), 0.0 );
                                    materials[ 3 ] = createMetalMaterial( vec3( 0.8, 0.6, 0.2 ), 0.0 );

                                    spheres[ 0 ] = createSphere( vec3( 0, -100.5, -1.0 ), 100.0, 0 );
                                    spheres[ 1 ] = createSphere( vec3( 0.0, 0.0, -1.0 ), 0.5, 1 );
                                    spheres[ 2 ] = createSphere( vec3( -1.0, 0.0, -1.0 ), 0.5, 2 );
                                    spheres[ 3 ] = createSphere( vec3( 1.0, 0.0, -1.0 ), 0.5, 3 );

                                    vec2 uv = gl_GlobalInvocationID.xy;
                                    uv += vec2( getRandom(), getRandom() );
                                    uv /= ( size.xy - vec2( 1 ) );
                                    uv.y = 1.0 - uv.y;
                                    Ray ray = getCameraRay( camera, uv.x, uv.y );
                                    vec3 color = rayColor( ray );

                                    // color = gammaCorrection( color, samplesPerPixel );

                                    vec3 destinationColor = imageLoad( resultImage, ivec2( gl_GlobalInvocationID.xy ) ).rgb;
                                    color = ( destinationColor * float( sampleCount - 1 ) + color ) / float( sampleCount );

                                    if ( sampleCount == 0 ) {
                                        color = vec3( 0 );
                                    }

                                    imageStore( resultImage, ivec2( gl_GlobalInvocationID.xy ), vec4( color, 1.0 ) );
                                } )" ) ),
                        1.0 ) );
            }() );
    }
};

CRIMILD_CREATE_SIMULATION( Example, "Compute: Raytracing" );
