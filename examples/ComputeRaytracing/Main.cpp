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
            computePass->setConditional( true );

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

        setComposition(
            [ & ] {
                using namespace crimild::compositions;
                return present(
                    computeImage(
                        width,
                        height,
                        crimild::alloc< Shader >(
                            Shader::Stage::COMPUTE,
                            R"(
                                layout( local_size_x = 32, local_size_y = 32 ) in;
                                layout( set = 0, binding = 0, rgba8 ) uniform image2D resultImage;

                                int seed = 0;
                                int flat_idx = 0;

                                struct Sphere {
                                    vec3 center;
                                    float radius;
                                };

                                Sphere createSphere( vec3 center, float radius )
                                {
                                    Sphere s;
                                    s.center = center;
                                    s.radius = radius;
                                    return s;
                                }

                                struct HitRecord {
                                    bool hasResult;
                                    float t;
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

                                Sphere spheres[ 2 ];

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
                                    return r.x * r.y;
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

                                HitRecord setFaceNormal( Ray ray, vec3 N, HitRecord rec )
                                {
                                    rec.frontFace = dot( ray.direction, N ) < 0;
                                    rec.normal = rec.frontFace ? N : -N;
                                    return rec;
                                }

                                vec3 rayAt( Ray ray, float t ) {
                                    return ray.origin + t * ray.direction;
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
                                    rec.point = rayAt( ray, root );
                                    vec3 normal = ( rec.point - sphere.center ) / sphere.radius;
                                    return setFaceNormal( ray, normal, rec );
                                }

                                HitRecord hitScene( Ray ray, float tMin, float tMax )
                                {
                                    HitRecord hit;
                                    hit.t = tMax;
                                    for ( int i = 0; i < 2; i++ ) {
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
                                    camera.origin = vec3( 0, 0, 0 );
                                    camera.horizontal = vec3( camera.viewport.x, 0, 0 );
                                    camera.vertical = vec3( 0, camera.viewport.y, 0 );
                                    camera.lowerLeftCorner = camera.origin - camera.horizontal / 2.0 - camera.vertical / 2.0 - vec3( 0, 0, camera.focalLength );
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
                                    int maxDepth = 20;

                                    float multiplier = 1.0;
                                    HitRecord hit = hitScene( ray, 0.001, 9999.9 );

                                    int depth = 0;
                                    while ( depth < maxDepth && hit.hasResult ) {
                                        vec3 target = hit.point + hit.normal + getRandomInUnitSphere();
                                        ray.origin = hit.point;
                                        ray.direction = normalize( target - hit.point );
                                        hit = hitScene( ray, 0.001, 9999.9 );
                                        multiplier *= 0.5;
                                        ++depth;
                                    }

                                    if ( depth >= maxDepth ) {
                                        return vec3( 0 );
                                    }

                                    vec3 D = normalize( ray.direction );
                                    float t = 0.5 * ( D.y + 1.0 );
                                    vec3 color = ( 1.0 - t ) * vec3( 1.0, 1.0, 1.0 ) + t * vec3( 0.5, 0.7, 1.0 );
                                    return multiplier * color;

                                    // HitRecord hit = hitScene( ray, 0.0, 9999.9 );
                                    // if ( hit.hasResult ) {
                                    //     vec3 target = hit.point + hit.normal + getRandomInUnitSphere();
                                    //     ray.origin = hit.point;
                                    //     ray.direction = target - hit.point;
                                    //     return 0.5 * rayColor( ray );
                                    // }

                                    // vec3 D = normalize( ray.direction );
                                    // float t = 0.5 * ( D.y + 1.0 );
                                    // return ( 1.0 - t ) * vec3( 1.0, 1.0, 1.0 ) + t * vec3( 0.5, 0.7, 1.0 );
                                }

                                void main() {
                                    seed = 0;
                                    flat_idx = int(dot(gl_GlobalInvocationID.xy, vec2(1, 4096)));

                                    int samplesPerPixel = 100;

                                    vec2 size = imageSize( resultImage );
                                    float aspectRatio = size.x / size.y;

                                    if ( gl_GlobalInvocationID.x >= size.x || gl_GlobalInvocationID.y >= size.y ) {
                                        return;
                                    }

                                    Camera camera = createCamera( aspectRatio );

                                    spheres[ 0 ] = createSphere( vec3( 0.0, 0.0, -1.0 ), 0.5 );
                                    spheres[ 1 ] = createSphere( vec3( 0, -100.5, -1.0 ), 100.0 );

                                    vec3 color = vec3( 0 );
                                    for ( int s = 0; s < samplesPerPixel; ++s ) {
                                        vec2 uv = gl_GlobalInvocationID.xy;
                                        uv += vec2( getRandom(), getRandom() );
                                        uv /= ( size.xy - vec2( 1 ) );
                                        uv.y = 1.0 - uv.y;
                                        Ray ray = getCameraRay( camera, uv.x, uv.y );
                                        color += rayColor( ray );
                                    }
                                    color /= vec3( samplesPerPixel );

                                    imageStore( resultImage, ivec2( gl_GlobalInvocationID.xy ), vec4( color, 1.0 ) );
                                } )" ) ) );
            }() );
    }
};

CRIMILD_CREATE_SIMULATION( Example, "Compute: Raytracing" );
