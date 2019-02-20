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

using namespace crimild;
using namespace crimild::rendergraph;
using namespace crimild::sdl;

SharedPointer< Node > buildLight( const Quaternion4f &rotation, const RGBAColorf &color, float major, float minor, float speed )
{
	auto orbitingLight = crimild::alloc< Group >();

	auto primitive = crimild::alloc< SpherePrimitive >( 0.05f );
	auto geometry = crimild::alloc< Geometry >();
	geometry->attachPrimitive( primitive );

	auto material = crimild::alloc< Material >();
	material->setDiffuse( color );
	material->setProgram( crimild::alloc< UnlitShaderProgram >() );
	geometry->getComponent< MaterialComponent >()->attachMaterial( material );

	orbitingLight->attachNode( geometry );

	auto light = crimild::alloc< Light >();
	light->setColor( color );
	light->setAttenuation( Vector3f( 0.0f, 0.5f, 3.0f ) );
	orbitingLight->attachNode( light );

	auto orbitComponent = crimild::alloc< OrbitComponent >( 0.0f, 0.0f, major, minor, speed );
	orbitingLight->attachComponent( orbitComponent );

	auto group = crimild::alloc< Group >();
	group->attachNode( orbitingLight );
	group->local().setRotate( rotation );

	return group;
}

int main( int argc, char **argv )
{
	crimild::init();
	
	CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< SDLSimulation >( "Lighting", crimild::alloc< Settings >( argc, argv ) );

	auto trefoilKnot = crimild::alloc< Geometry >();
	auto trefoilKnotPrimitive = crimild::alloc< TrefoilKnotPrimitive >(
		Primitive::Type::TRIANGLES,
		1.0,
		VertexFormat::VF_P3_N3,
		Vector2f( 300, 60 )
	);
	trefoilKnot->attachPrimitive( trefoilKnotPrimitive );
	
	auto material = crimild::alloc< Material >();
	material->setAmbient( RGBAColorf( 0.0f, 0.0f, 0.0f, 1.0f ) );
	material->setDiffuse( RGBAColorf( 1.0f, 1.0f, 1.0f, 1.0f ) );
	trefoilKnot->getComponent< MaterialComponent >()->attachMaterial( material );

	auto scene = crimild::alloc< Group >();
	scene->attachNode( trefoilKnot );

	scene->attachNode( buildLight( 
		Quaternion4f::createFromAxisAngle( Vector3f( 0.0f, 1.0f, 1.0 ).getNormalized(), Numericf::HALF_PI ), 
		RGBAColorf( 1.0f, 0.0f, 0.0f, 1.0f ), 
		-1.0f, 1.25f, 0.95f ).get() );

	scene->attachNode( buildLight( 
		Quaternion4f::createFromAxisAngle( Vector3f( 0.0f, 1.0f, -1.0 ).getNormalized(), -Numericf::HALF_PI ), 
		RGBAColorf( 0.0f, 1.0f, 0.0f, 1.0f ), 
		1.0f, 1.25f, -0.75f ).get() );

	scene->attachNode( buildLight( 
		Quaternion4f::createFromAxisAngle( Vector3f( 0.0f, 1.0f, 0.0 ).getNormalized(), Numericf::HALF_PI ), 
		RGBAColorf( 0.0f, 0.0f, 1.0f, 1.0f ), 
		1.25f, 1.0f, -0.85f ).get() );

	auto camera = crimild::alloc< Camera >( 45.0f, 4.0f / 3.0f, 0.1f, 1024.0f );
	camera->local().setTranslate( 0.0f, 0.0f, 3.0f );
	scene->attachNode( camera );

	sim->setScene( scene );
	return sim->run();
}

