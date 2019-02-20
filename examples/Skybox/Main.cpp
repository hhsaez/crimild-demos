/*
 * Copyright (c) 2002-present, H. Hernan Saez
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

#define CRIMILD_ENABLE_CHECK_GL_ERRORS 1

#include <Crimild.hpp>
#include <Crimild_SDL.hpp>

using namespace crimild;
using namespace crimild::sdl;
using namespace crimild::rendergraph;
using namespace crimild::rendergraph::passes;

SharedPointer< RenderGraph > createRenderGraph( void )
{
	auto graph = crimild::alloc< RenderGraph >();

	auto depthPass = graph->createPass< DepthPass >();
	auto scenePass = graph->createPass< ForwardLightingPass >();
	auto skyboxPass = graph->createPass< SkyboxPass >();
	auto blend = graph->createPass< BlendPass >();

	scenePass->setDepthInput( depthPass->getDepthOutput() );
    skyboxPass->setDepthInput( depthPass->getDepthOutput() );

	blend->addInput( scenePass->getColorOutput() );
	blend->addInput( skyboxPass->getColorOutput() );
	
    graph->setOutput( blend->getOutput() );

	return graph;
}

int main( int argc, char **argv )
{
	auto sim = crimild::alloc< SDLSimulation >( "Skybox", crimild::alloc< Settings >( argc, argv ) );

	auto scene = crimild::alloc< Group >();

	auto box = crimild::alloc< Geometry >();
	box->attachPrimitive( crimild::alloc< BoxPrimitive >( 1.0f, 1.0f, 1.0f ) );
	auto boxMaterial = crimild::alloc< Material >();
	boxMaterial->setAmbient( RGBAColorf::ONE );
	box->getComponent< MaterialComponent >()->attachMaterial( boxMaterial );
	scene->attachNode( box );

	auto sun = crimild::alloc< Light >( Light::Type::DIRECTIONAL );
	sun->local().rotate().fromEulerAngles( -0.25f * Numericf::PI, -0.75f * Numericf::PI, 0.0f );
	scene->attachNode( sun );

	auto ambient = crimild::alloc< Light >( Light::Type::AMBIENT );
	ambient->setAmbient( RGBAColorf( 0.0f, 0.0f, 0.1f, 1.0f ) );
	scene->attachNode( ambient );

	scene->attachNode(
		crimild::alloc< Skybox >(
			containers::Array< SharedPointer< Image >> {
				crimild::alloc< ImageTGA >( FileSystem::getInstance().pathForResource( "assets/textures/right.tga" ) ),
				crimild::alloc< ImageTGA >( FileSystem::getInstance().pathForResource( "assets/textures/left.tga" ) ),
				crimild::alloc< ImageTGA >( FileSystem::getInstance().pathForResource( "assets/textures/top.tga" ) ),
				crimild::alloc< ImageTGA >( FileSystem::getInstance().pathForResource( "assets/textures/bottom.tga" ) ),
				crimild::alloc< ImageTGA >( FileSystem::getInstance().pathForResource( "assets/textures/back.tga" ) ),
				crimild::alloc< ImageTGA >( FileSystem::getInstance().pathForResource( "assets/textures/front.tga" ) ),
			}
		)
	);

	auto camera = crimild::alloc< Camera >();
	camera->local().setTranslate( 0.f, 0.0f, 5.0f );
	camera->attachComponent< FreeLookCameraComponent >();
    camera->setRenderGraph( createRenderGraph() );
	scene->attachNode( camera );

	sim->setScene( scene );

	return sim->run();
}

