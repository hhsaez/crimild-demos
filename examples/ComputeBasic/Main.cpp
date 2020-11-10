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
#include <Crimild_Vulkan.hpp>

using namespace crimild;
using namespace crimild::glfw;
using namespace crimild::vulkan;

const auto WIDTH = 1000;
const auto HEIGHT = 1000;
const auto WORKGROUP_SIZE = 32;

class ExampleVulkanSystem : public GLFWVulkanSystem {
public:
    crimild::Bool start( void ) override
    {
        if ( !GLFWVulkanSystem::start() ) {
            return false;
        }

        m_frameGraph = crimild::alloc< FrameGraph >();

        auto data = Array< RGBAColorf >( WIDTH * HEIGHT ).fill( []( auto i ) { return RGBAColorf( 0.0f, 1.0f, 0.0f, 1.0f ); } );
        m_buffer = crimild::alloc< StorageBuffer >( data );

        m_pipeline = [ & ] {
            auto pipeline = crimild::alloc< ComputePipeline >();
            pipeline->setProgram(
                [] {
                    auto program = crimild::alloc< ShaderProgram >(
                        Array< SharedPointer< Shader > > {
                            Shader::withSource(
                                Shader::Stage::COMPUTE,
                                { .path = "main.comp" } ),
                        } );
                    program->descriptorSetLayouts = {
                        [] {
                            auto layout = crimild::alloc< DescriptorSetLayout >();
                            layout->bindings = {
                                {
                                    .descriptorType = DescriptorType::STORAGE_BUFFER,
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

        m_descriptors = [ & ] {
            auto ds = crimild::alloc< DescriptorSet >();
            ds->descriptors = {
                Descriptor {
                    .descriptorType = DescriptorType::STORAGE_BUFFER,
                    .obj = m_buffer,
                },
            };
            return ds;
        }();

        auto commandBuffer = crimild::alloc< CommandBuffer >();
        commandBuffer->begin( CommandBuffer::Usage::ONE_TIME_SUBMIT );
        commandBuffer->bindComputePipeline( crimild::get_ptr( m_pipeline ) );
        commandBuffer->bindDescriptorSet( crimild::get_ptr( m_descriptors ) );
        commandBuffer->dispatch(
            DispatchWorkgroup {
                .x = Numeric< UInt32 >::ceil( Real32( WIDTH ) / Real32( WORKGROUP_SIZE ) ),
                .y = Numeric< UInt32 >::ceil( Real32( WIDTH ) / Real32( WORKGROUP_SIZE ) ),
                .z = 1,
            } );
        commandBuffer->end();

        m_frameGraph->compile();

        auto renderDevice = getRenderDevice();
        renderDevice->submitComputeCommands( crimild::get_ptr( commandBuffer ) );
        renderDevice->waitIdle();

        return true;
    }

    void update( void ) override
    {
        GLFWVulkanSystem::update();

        // Required
        Simulation::getInstance()->stop();
    }

    void stop( void ) override
    {
        if ( auto renderDevice = getRenderDevice() ) {
            renderDevice->waitIdle();
        }

        saveImage();

        m_frameGraph = nullptr;

        GLFWVulkanSystem::stop();
    }

private:
    void saveImage( void ) noexcept
    {
        auto renderDevice = getRenderDevice();
        if ( renderDevice == nullptr ) {
            return;
        }

        std::cout << "Getting image data" << std::endl;
        renderDevice->mapFromDevice( crimild::get_ptr( m_buffer ) );

        auto outputPath = FileSystem::getInstance().pathForDocument( "screenshot.ppm" );
        std::ofstream out( outputPath, std::ios::out );

        out << "P3"
            << "\n"
            << int( WIDTH ) << " " << int( HEIGHT )
            << "\n255"
            << "\n";

        std::cout << "\nSaving " << outputPath << std::endl;
        m_buffer->each< RGBAColorf >(
            [ &out ]( const auto &p, auto index ) {
                out << Int32( p.r() * 255.0 ) << " " << Int32( p.g() * 255.0 ) << " " << Int32( p.b() * 255.0 ) << "\n";
            } );
    }

private:
    SharedPointer< FrameGraph > m_frameGraph;
    SharedPointer< StorageBuffer > m_buffer;
    SharedPointer< ComputePipeline > m_pipeline;
    SharedPointer< DescriptorSet > m_descriptors;
    SharedPointer< ComputePass > m_computePass;
};

int main( int argc, char **argv )
{
    crimild::init();
    crimild::vulkan::init();

    Log::setLevel( Log::Level::LOG_LEVEL_ALL );

    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< GLSimulation >( "Compute: Basic", crimild::alloc< Settings >( argc, argv ) );
    sim->addSystem( crimild::alloc< ExampleVulkanSystem >() );
    return sim->run();
}
