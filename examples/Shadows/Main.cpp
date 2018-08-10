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

#include <fstream>
#include <string>
#include <vector>

using namespace crimild;
using namespace crimild::sdl;

SharedPointer< Node > loadScene( void )
{
    auto scene = crimild::alloc< Group >( "scene" );
    
	OBJLoader loader( FileSystem::getInstance().pathForResource( "assets/scene.obj" ) );
	auto model = loader.load();
	if ( model != nullptr ) {
		scene->attachNode( model );
		scene->attachComponent( crimild::alloc< RotationComponent >( Vector3f::UNIT_Y, 0.1f ) );
	}
    
    return scene;
}

int main( int argc, char **argv )
{
	auto sim = crimild::alloc< SDLSimulation >( "Shadows", crimild::alloc< Settings >( argc, argv ) );

	auto scene = crimild::alloc< Group >();
    scene->attachNode( loadScene() );

	auto light = crimild::alloc< Light >( Light::Type::DIRECTIONAL );
	light->local().setTranslate( 10.0f, 25.0f, 20.0f );
    light->local().lookAt( Vector3f( 0.0f, 0.0f, 0.0f ), Vector3f( 0.0f, 1.0f, 0.0 ) );
    light->setShadowMap( crimild::alloc< ShadowMap >() );
	scene->attachNode( light );

	auto camera = crimild::alloc< Camera >( 45.0f, 4.0f / 3.0f, 0.1f, 1024.0f );
	camera->local().setTranslate( 1.0f, 25.0f, 25.0f );
    camera->local().lookAt( Vector3f( 0.0f, 5.0f, 0.0 ), Vector3f( 0.0f, 1.0f, 0.0f ) );
    auto renderPass = crimild::alloc< CompositeRenderPass >();
    renderPass->attachRenderPass( crimild::alloc< ShadowRenderPass >() );
    renderPass->attachRenderPass( crimild::alloc< StandardRenderPass >() );
    camera->setRenderPass( renderPass );
	scene->attachNode( camera );

	sim->setScene( scene );
	return sim->run();
}

