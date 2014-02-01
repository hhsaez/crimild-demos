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
#include <Crimild_COLLADA.hpp>

using namespace crimild;

int main( int argc, char **argv )
{
	Pointer< Simulation > sim( new GLSimulation( "Loading scenes from COLLADA files", argc, argv ) );

	std::string filePath = "astroBoy_walk_Max.dae";
	if ( argc >= 2 ) {
		filePath = argv[ 1 ];
	}

	collada::COLLADALoader loader( FileSystem::getInstance().pathForResource( filePath ) );
	Pointer< Node > model = loader.load();
	model->local().setRotate( Vector3f( 1.0f, 0.0f, 0.0f ), -Numericf::HALF_PI );
	model->perform( UpdateWorldState() );

	Pointer< Group > importedScene( new Group() );
	importedScene->attachNode( model );

	Pointer< Group > scene( new Group() );
	scene->attachNode( importedScene );

	Pointer< Camera > camera( new Camera() );
	camera->local().setTranslate( model->getWorldBound()->getCenter() + Vector3f( 0.0f, 0.0f, model->getWorldBound()->getRadius() ) );
	Pointer< RenderPass > defaultRP( new RenderPass() );
	Pointer< HierarchyRenderPass > debugRP( new HierarchyRenderPass( defaultRP ) );
	debugRP->setTargetScene( model );
	debugRP->setRenderBoundings( true );
	camera->setRenderPass( debugRP );
	scene->attachNode( camera );

	Pointer< Light > light( new Light() );
	light->setLocal( camera->getLocal() );
	scene->attachNode( light );

	Pointer< LambdaComponent > controls( new LambdaComponent( [&]( crimild::Node *, const Time &t ) {
		if ( InputState::getCurrentState().isKeyStillDown( '1' ) ) {
			camera->setRenderPass( defaultRP );
		}

		if ( InputState::getCurrentState().isKeyStillDown( '2' ) ) {
			camera->setRenderPass( debugRP );
		}

		if ( InputState::getCurrentState().isKeyStillDown( 'W' ) ) {
			importedScene->local().translate() += Vector3f( 0.0f, 0.0f, t.getDeltaTime() );
		}
		if ( InputState::getCurrentState().isKeyStillDown( 'S' ) ) {
			importedScene->local().translate() += Vector3f( 0.0f, 0.0f, -t.getDeltaTime() );
		}
		if ( InputState::getCurrentState().isKeyStillDown( 'A' ) ) {
			importedScene->local().rotate() *= Quaternion4f::createFromAxisAngle( Vector3f( 0.0f, 1.0f, 0.0f ), t.getDeltaTime() );
		}
		if ( InputState::getCurrentState().isKeyStillDown( 'D' ) ) {
			importedScene->local().rotate() *= Quaternion4f::createFromAxisAngle( Vector3f( 0.0f, 1.0f, 0.0f ), -t.getDeltaTime() );
		}
	}));
	scene->attachComponent( controls );

	sim->setScene( scene );
	return sim->run();
}

