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
#include <Crimild_GLFW.hpp>

using namespace crimild;

int main( int argc, char **argv )
{
	auto sim = crimild::alloc< GLSimulation >( "IronMan", crimild::alloc< Settings >( argc, argv ) );

	auto scene = crimild::alloc< Group >();

	OBJLoader loader( FileSystem::getInstance().pathForResource( "assets/Iron_Man.obj" ) );
	auto ironman = loader.load();
	if ( ironman != nullptr ) {
		auto rotationComponent = crimild::alloc< RotationComponent >( Vector3f( 0, 1, 0 ), 0.01 );
		ironman->attachComponent( rotationComponent );
		scene->attachNode( ironman );
	}

	auto light = crimild::alloc< Light >();
	light->local().setTranslate( 5.0f, 4.0f, 10.0f );
    light->local().lookAt( Vector3f( 0.0f, 0.0f, 0.0f ), Vector3f( 0.0f, 1.0f, 0.0f ) );
    light->setCastShadows( true );
    light->setShadowNearCoeff( 1.0f );
    light->setShadowFarCoeff( 50.0f );
	scene->attachNode( light );

	auto camera = crimild::alloc< Camera >();
    //camera->setRenderPass( crimild::alloc< DeferredRenderPass >() );
    // camera->getRenderPass()->getImageEffects().add( new gl3::GlowImageEffect() );
	camera->local().setTranslate( 0.0f, 2.88f, 3.5f );
	scene->attachNode( camera );

	sim->setScene( scene );

	return sim->run();
}

