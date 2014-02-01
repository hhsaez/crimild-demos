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

Pointer< Node > makeBall( float x, float y, float z, float fx = 0.0f, float fy = 0.0f, float fz = 0.0f )
{
	Pointer< Primitive > sphere( new SpherePrimitive( 1.0 ) );
	Pointer< Geometry > geometry( new Geometry() );
	geometry->attachPrimitive( sphere );
	geometry->local().setTranslate( x, y, z );

	Pointer< RigidBodyComponent > rigidBody( new RigidBodyComponent() );
	rigidBody->setForce( Vector3f( fx, fy, fz ) );
	geometry->attachComponent( rigidBody );

	Pointer< ColliderComponent > collider( new ColliderComponent() );
	geometry->attachComponent( collider );

	return geometry;
}

Pointer< Node > makeGround( void )
{
	Pointer< Primitive > primitive( new QuadPrimitive( 10.0f, 10.0f ) );
	Pointer< Geometry > geometry( new Geometry() );
	geometry->attachPrimitive( primitive );
	geometry->local().setRotate( Vector3f( 1.0f, 0.0f, 0.0f ), -Numericf::HALF_PI );
	
	Pointer< PlaneBoundingVolume > planeBoundingVolume( new PlaneBoundingVolume( Plane3f( Vector3f( 0.0f, 1.0f, 0.0f ), 0.0f ) ) );
	Pointer< ColliderComponent > collider( new ColliderComponent( planeBoundingVolume ) );
	geometry->attachComponent( collider );

	return geometry;
}

int main( int argc, char **argv )
{
	Pointer< Simulation > sim( new GLSimulation( "Physics", argc, argv ) );

	Pointer< Group > scene( new Group() );
	scene->attachNode( makeGround() );
	scene->attachNode( makeBall( -2.0f, 1.0f, 0.0f, 5.0f, 2.0f ) );
	scene->attachNode( makeBall( 2.0f, 5.0f, 0.0f ) );

	Pointer< Camera > camera( new Camera() );
	camera->local().setTranslate( 0.0f, 3.0f, 10.0f );
	camera->local().lookAt( Vector3f( 0.0f, 0.0f, 0.0f ), Vector3f( 0.0f, 1.0f, 0.0f ) );
	scene->attachNode( camera );

	Pointer< Light > light( new Light() );
	light->local().setTranslate( 0.0f, 5.0f, 5.0f );
	scene->attachNode( light );

	sim->setScene( scene );

	return sim->run();
}

