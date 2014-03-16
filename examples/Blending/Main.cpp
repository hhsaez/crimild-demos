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

Pointer< Node > buildBackground( float x, float y, float z ) 
{
	Pointer< Primitive > primitive( new QuadPrimitive( 9.0f, 9.0f, VertexFormat::VF_P3_N3_UV2 ) );
	Pointer< Geometry > geometry( new Geometry() );
	geometry->attachPrimitive( primitive.get() );

	Pointer< Material > material( new Material() );
	Pointer< Image > image( new ImageTGA( FileSystem::getInstance().pathForResource( "stars.tga" ) ) );
	Pointer< Texture > texture( new Texture( image.get() ) );
	material->setColorMap( texture.get() );
	
	Pointer< MaterialComponent > materials( new MaterialComponent() );
	materials->attachMaterial( material.get() );
	geometry->attachComponent( materials.get() );

	geometry->local().setTranslate( x, y, z );

	return geometry;	
}

Pointer< Node > buildEarth( float x, float y, float z )
{
	Pointer< Primitive > primitive( new SpherePrimitive( 1.0f, VertexFormat::VF_P3_N3_UV2 ) );
	Pointer< Geometry > geometry( new Geometry() );
	geometry->attachPrimitive( primitive.get() );

	Pointer< Material > material( new Material() );
	Pointer< Image > image( new ImageTGA( FileSystem::getInstance().pathForResource( "earth-color.tga" ) ) );
	Pointer< Texture > texture( new Texture( image.get() ) );
	material->setColorMap( texture.get() );
	
	Pointer< MaterialComponent > materials( new MaterialComponent() );
	materials->attachMaterial( material.get() );
	geometry->attachComponent( materials.get() );

	Pointer< NodeComponent > rotation( new RotationComponent( Vector3f( 0.0f, 1.0f, 0.0f ), 0.001 ) );
	geometry->attachComponent( rotation.get() );

	geometry->local().setTranslate( x, y, z );

	return geometry;
}

Pointer< Node > buildAtmosphere( float x, float y, float z )
{
	Pointer< Primitive > primitive( new SpherePrimitive( 1.02f, VertexFormat::VF_P3_N3_UV2 ) );
	Pointer< Geometry > geometry( new Geometry() );
	geometry->attachPrimitive( primitive.get() );

	Pointer< Material > material( new Material() );
	Pointer< Image > image( new ImageTGA( FileSystem::getInstance().pathForResource( "earth-atmosphere.tga" ) ) );
	Pointer< Texture > texture( new Texture( image.get() ) );
	material->setColorMap( texture.get() );
	material->getAlphaState()->setEnabled( true );
    material->setAmbient( RGBAColorf( 0.35f, 0.35f, 0.35f, 1.0f ) );
	material->getAlphaState()->setSrcBlendFunc( AlphaState::SrcBlendFunc::SRC_COLOR );
	material->getAlphaState()->setDstBlendFunc( AlphaState::DstBlendFunc::ONE_MINUS_SRC_COLOR );
	geometry->getComponent< MaterialComponent >()->attachMaterial( material.get() );

	Pointer< NodeComponent > rotation( new RotationComponent( Vector3f( 0.0f, 1.0f, 0.0f ), 0.005 ) );
	geometry->attachComponent( rotation.get() );

	geometry->local().setTranslate( x, y, z );

	return geometry;
}

int main( int argc, char **argv )
{
	Pointer< Simulation > sim( new GLSimulation( "Textures", argc, argv ) );

	Pointer< Group > scene( new Group() );
	scene->attachNode( buildBackground( 0, 0, -5 ).get() );
	scene->attachNode( buildEarth( 0.5, 0, 0 ).get() );
	scene->attachNode( buildAtmosphere( 0.5, 0, 0 ).get() );

	Pointer< Camera > camera( new Camera() );
	camera->local().setTranslate( 0.0f, 0.0f, 3.0f );
	scene->attachNode( camera.get() );
    
    Light *light = new Light();
    light->local().setTranslate( -3.0f, 1.0f, 3.0f );
    scene->attachNode( light );

	sim->setScene( scene.get() );
	return sim->run();
}

