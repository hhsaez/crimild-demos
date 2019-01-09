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

#include <Crimild.hpp>
#include <Crimild_SDL.hpp>

#include "Rendering/RenderGraph/RenderGraph.hpp"
#include "Rendering/RenderGraph/Passes/DepthPass.hpp"
#include "Rendering/RenderGraph/Passes/LinearizeDepthPass.hpp"

using namespace crimild;
using namespace crimild::rendergraph;
using namespace crimild::sdl;

SharedPointer< RenderGraph > createRenderGraph( void )
{
	auto graph = crimild::alloc< RenderGraph >();

	auto depthPass = graph->createPass< passes::DepthPass >();
    auto linearizeDepthPass = graph->createPass< passes::LinearizeDepthPass >();

	linearizeDepthPass->setInput( depthPass->getDepthOutput() );	

	graph->setOutput( linearizeDepthPass->getOutput() );

	return graph;
}

int main( int argc, char **argv )
{
	auto sim = crimild::alloc< SDLSimulation >( "Depth Buffer", crimild::alloc< Settings >( argc, argv ) );

	auto scene = crimild::alloc< Group >();

	OBJLoader loader( FileSystem::getInstance().pathForResource( "assets/models/sponza/sponza.obj" ) );
	auto model = loader.load();
	if ( model != nullptr ) {
		scene->attachNode( model );
	}

	auto camera = crimild::alloc< Camera >( 45.0f, 4.0f / 3.0f, 1.0f, 3000.0f );
	camera->local().setTranslate( 0.0f, 200.0f, 400.0f );
	camera->local().rotate().fromEulerAngles( 0.0f, -0.5f * Numericf::PI, 0.0f );
    camera->setRenderPass( crimild::alloc< RenderGraphRenderPass >( createRenderGraph() ) );
	scene->attachNode( camera );

	sim->setScene( scene );

	return sim->run();
}

