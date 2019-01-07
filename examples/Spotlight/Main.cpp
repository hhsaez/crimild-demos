/*
 * Copyright (c) 2002-present, H. Hernán Saez
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

#include <Crimild.hpp>
#include <Crimild_SDL.hpp>
#include <Crimild_OpenGL.hpp>

#include "Rendering/RenderGraph/RenderGraph.hpp"
#include "Rendering/RenderGraph/RenderGraphPass.hpp"
#include "Rendering/RenderGraph/RenderGraphAttachment.hpp"
#include "Rendering/RenderGraph/Passes/ForwardLightingPass.hpp"
#include "Rendering/RenderGraph/Passes/ScreenPass.hpp"
#include "Rendering/RenderGraph/Passes/DepthPass.hpp"
#include "Rendering/RenderGraph/Passes/BlendPass.hpp"
#include "Rendering/RenderGraph/Passes/DepthToRGBPass.hpp"
#include "Rendering/RenderGraph/Passes/FrameDebugPass.hpp"
#include "Rendering/RenderGraph/Passes/OpaquePass.hpp"
#include "Rendering/RenderGraph/Passes/LightAccumulationPass.hpp"
#include "Rendering/RenderGraph/Passes/TextureColorPass.hpp"
#include "Rendering/RenderGraph/Passes/DeferredLightingPass.hpp"

using namespace crimild;
using namespace crimild::rendergraph;
using namespace crimild::rendergraph::passes;

SharedPointer< RenderGraph > createRenderGraph( void )
{
	auto graph = crimild::alloc< RenderGraph >();
	auto scenePass = graph->createPass< passes::ForwardLightingPass >();
	graph->setOutput( scenePass->getColorOutput() );

	return graph;
}

SharedPointer< Node > buildAmbientLight( const RGBAColorf &color )
{
	auto light = crimild::alloc< Light >( Light::Type::AMBIENT );
	light->setAmbient( color );
	return light;
}

SharedPointer< Node > buildSpotlight( void )
{
	auto light = crimild::alloc< Light >( Light::Type::SPOT );
	light->setInnerCutoff( Numericf::DEG_TO_RAD * 17.0f );
	light->setOuterCutoff( Numericf::DEG_TO_RAD * 25.0f );

	light->setColor( RGBAColorf( 1.0f, 0.0f, 0.0f, 1.0f ) );

	return light;
}

SharedPointer< Node > buildCube( const Vector3f &position )
{
	auto geometry = crimild::alloc< Geometry >();
	geometry->attachPrimitive( crimild::alloc< BoxPrimitive >( 1.0f, 1.0f, 1.0f, VertexFormat::VF_P3_N3 ) );

	auto material = crimild::alloc< Material >();
	material->setAmbient( RGBAColorf::ONE );
	material->setDiffuse( RGBAColorf::ONE );
	geometry->getComponent< MaterialComponent >()->attachMaterial( material );

	geometry->local().setTranslate( position );

	return geometry;
}

int main( int argc, char **argv )
{
    auto settings = crimild::alloc< Settings >( argc, argv );
    settings->set( "video.show_frame_time", true );
    auto sim = crimild::alloc< sdl::SDLSimulation >( "Spotlight", settings );

    auto scene = crimild::alloc< Group >();

	scene->attachNode( buildCube( Vector3f( 0.0f, 0.5f, -1.0f ) ) );
	scene->attachNode( buildCube( Vector3f( -1.0f, 0.5f, 1.0f ) ) );
	scene->attachNode( buildCube( Vector3f( 1.0f, -0.5f, 1.0f ) ) );

	auto camera = crimild::alloc< Camera >();
	camera->local().setTranslate( 0.0f, 3.0f, 3.0f );
	camera->local().lookAt( Vector3f::ZERO );
    auto renderGraph = createRenderGraph();
    camera->setRenderPass( crimild::alloc< RenderGraphRenderPass >( renderGraph ) );
	scene->attachNode( camera );
    
	scene->attachNode( buildAmbientLight( RGBAColorf( 0.1f, 0.0f, 0.05f, 1.0f ) ) );
	camera->attachNode( buildSpotlight() );

    sim->setScene( scene );
	
	return sim->run();
}

