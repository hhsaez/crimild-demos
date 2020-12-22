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

        Composition computeImage( void ) noexcept
        {
            Composition cmp;

            const UInt32 width = 1024;
            const UInt32 height = 1024;

            auto texture = cmp.create< Texture >();
            texture->imageView = [ & ] {
                auto imageView = crimild::alloc< ImageView >();
                imageView->image = [ & ] {
                    auto image = crimild::alloc< Image >();
                    image->format = Format::R8G8B8A8_UNORM;
                    image->extent = {
                        .width = width,
                        .height = height,
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
                    [] {
                        auto program = crimild::alloc< ShaderProgram >(
                            Array< SharedPointer< Shader > > {
                                crimild::alloc< Shader >(
                                    Shader::Stage::COMPUTE,
                                    CRIMILD_TO_STRING(
                                        layout( local_size_x = 32, local_size_y = 32 ) in;
                                        layout( set = 0, binding = 0, rgba8 ) uniform image2D resultImage;

                                        void main() {
                                            vec2 size = imageSize( resultImage );

                                            if ( gl_GlobalInvocationID.x >= size.x || gl_GlobalInvocationID.y >= size.y ) {
                                                return;
                                            }

                                            float x = float( gl_GlobalInvocationID.x ) / size.x;
                                            float y = float( gl_GlobalInvocationID.y ) / size.y;

                                            vec2 uv = vec2( x, y );
                                            float n = 0.0;
                                            vec2 c = vec2( -.445, 0.0 ) + ( uv - 0.5 ) * ( 2.0 + 1.7 * 0.2 );
                                            vec2 z = vec2( 0.0 );
                                            const int M = 128;
                                            for ( int i = 0; i < M; i++ ) {
                                                z = vec2( z.x * z.x - z.y * z.y, 2. * z.x * z.y ) + c;
                                                if ( dot( z, z ) > 2 )
                                                    break;
                                                n++;
                                            }

                                            float t = float( n ) / float( M );
                                            vec3 d = vec3( 0.3, 0.3, 0.5 );
                                            vec3 e = vec3( -0.2, -0.3, -0.5 );
                                            vec3 f = vec3( 2.1, 2.0, 3.0 );
                                            vec3 g = vec3( 0.0, 0.1, 0.0 );
                                            vec4 color = vec4( d + e * cos( 6.28318 * ( f * t + g ) ), 1.0 );

                                            imageStore( resultImage, ivec2( gl_GlobalInvocationID.xy ), color );
                                        } ) ),
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
        setComposition(
            [ & ] {
                using namespace crimild::compositions;
                return present( invert( computeImage() ) );
            }() );
    }
};

CRIMILD_CREATE_SIMULATION( Example, "Compute: Image" );
