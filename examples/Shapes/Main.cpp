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
	Pointer< Simulation > sim( new GLSimulation( "Default shapes", argc, argv ) );

	Pointer< Group > scene( new Group() );

	Pointer< Group > shapes( new Group() );
	scene->attachNode( shapes.get() );

	Pointer< Geometry > kleinBottle( new Geometry() );
	Pointer< Primitive > kleinBottlePrimitive( new KleinBottlePrimitive( Primitive::Type::LINES, 0.1 ) );
	kleinBottle->attachPrimitive( kleinBottlePrimitive.get() );
	kleinBottle->local().setTranslate( 0.0f, 0.0f, 3.0f );
	shapes->attachNode( kleinBottle.get() );

	Pointer< Geometry > mobiusStrip( new Geometry() );
	Pointer< Primitive > mobiusStripPrimitive( new MobiusStripPrimitive( Primitive::Type::LINES, 0.5f ) );
	mobiusStrip->attachPrimitive( mobiusStripPrimitive.get() );
	mobiusStrip->local().setTranslate( 0.0f, 0.0f, -3.0f );
	shapes->attachNode( mobiusStrip.get() );

	Pointer< Geometry > torus( new Geometry() );
	Pointer< Primitive > torusPrimitive( new TorusPrimitive( Primitive::Type::LINES, 1.0f, 0.25f ) );
	torus->attachPrimitive( torusPrimitive.get() );
	torus->local().setTranslate( 3.0f, 0.0f, 0.0f );
	shapes->attachNode( torus.get() );

	Pointer< Geometry > trefoilKnot( new Geometry() );
	Pointer< Primitive > trefoilKnotPrimitive( new TrefoilKnotPrimitive( Primitive::Type::LINES, 1.0 ) );
	trefoilKnot->attachPrimitive( trefoilKnotPrimitive.get() );
	trefoilKnot->local().setTranslate( -3.0f, 0.0f, 0.0f );
	shapes->attachNode( trefoilKnot.get() );

	Pointer< NodeComponent > rotate( new RotationComponent( Vector3f( 0.0f, 1.0f, 0.0f ), 0.1f ) );
	shapes->attachComponent( rotate.get() );

	Pointer< Camera > camera( new Camera() );
	camera->local().setTranslate( 0.0f, 0.0f, 10.0f );
	scene->attachNode( camera.get() );

	sim->setScene( scene.get() );
	return sim->run();
}

