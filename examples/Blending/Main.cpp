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

SharedPointer< Node > buildBackground( float x, float y, float z ) 
{
	auto primitive = crimild::alloc< QuadPrimitive >( 9.0f, 9.0f, VertexFormat::VF_P3_N3_UV2 );
	auto geometry = crimild::alloc< Geometry >();
	geometry->attachPrimitive( primitive );

	auto material = crimild::alloc< Material >();
	material->setColorMap( AssetManager::getInstance()->get< Texture >( "stars.tga" ) );
	geometry->getComponent< MaterialComponent >()->attachMaterial( material );

	geometry->local().setTranslate( x, y, z );

	return geometry;	
}

SharedPointer< Node > buildEarth( float x, float y, float z )
{
	auto primitive = crimild::alloc< SpherePrimitive >( 1.0f, VertexFormat::VF_P3_N3_UV2 );
	auto geometry = crimild::alloc< Geometry >();
	geometry->attachPrimitive( primitive );

	auto material = crimild::alloc< Material >();
	material->setColorMap( AssetManager::getInstance()->get< Texture >( "earth-color.tga" ) );
	geometry->getComponent< MaterialComponent >()->attachMaterial( material );

	geometry->attachComponent( crimild::alloc< RotationComponent >( Vector3f( 0.0f, 1.0f, 0.0f ), 0.001 ) );

	geometry->local().setTranslate( x, y, z );

	return geometry;
}

SharedPointer< Node > buildAtmosphere( float x, float y, float z )
{
	auto primitive = crimild::alloc< SpherePrimitive >( 1.02f, VertexFormat::VF_P3_N3_UV2 );
	auto geometry = crimild::alloc< Geometry >();
	geometry->attachPrimitive( primitive );

	auto material = crimild::alloc< Material >();
	material->setColorMap( AssetManager::getInstance()->get< Texture >( "earth-atmosphere.tga" ) );
	material->getAlphaState()->setEnabled( true );
    material->setAmbient( RGBAColorf( 0.35f, 0.35f, 0.35f, 1.0f ) );
	material->getAlphaState()->setSrcBlendFunc( AlphaState::SrcBlendFunc::SRC_COLOR );
	material->getAlphaState()->setDstBlendFunc( AlphaState::DstBlendFunc::ONE_MINUS_SRC_COLOR );
	geometry->getComponent< MaterialComponent >()->attachMaterial( material );

	geometry->attachComponent( crimild::alloc< RotationComponent >( Vector3f( 0.0f, 1.0f, 0.0f ), 0.005 ) );

	geometry->local().setTranslate( x, y, z );

	return geometry;
}

int main( int argc, char **argv )
{
	auto sim = crimild::alloc< GLSimulation >( "Textures", crimild::alloc< Settings >( argc, argv ) );

	auto scene = crimild::alloc< Group >();
	scene->attachNode( buildBackground( 0, 0, -5 ) );
	scene->attachNode( buildEarth( 0.5, 0, 0 ) );
	scene->attachNode( buildAtmosphere( 0.5, 0, 0 ) );

	auto camera = crimild::alloc< Camera >();
	camera->local().setTranslate( 0.0f, 0.0f, 3.0f );
	scene->attachNode( camera );
    
    auto light = crimild::alloc< Light >();
    light->local().setTranslate( -3.0f, 1.0f, 3.0f );
    scene->attachNode( light );

	sim->setScene( scene );

	return sim->run();
}

