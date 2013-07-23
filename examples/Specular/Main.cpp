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

NodePtr buildBackground( float x, float y, float z ) 
{
	PrimitivePtr primitive( new QuadPrimitive( 9.0f, 9.0f, VertexFormat::VF_P3_N3_UV2 ) );
	GeometryPtr geometry( new Geometry() );
	geometry->attachPrimitive( primitive );

	MaterialPtr material( new Material() );
	ImagePtr image( new ImageTGA( FileSystem::getInstance().pathForResource( "stars.tga" ) ) );
	TexturePtr texture( new Texture( image ) );
	material->setColorMap( texture );
	
	MaterialComponentPtr materials( new MaterialComponent() );
	materials->attachMaterial( material );
	geometry->attachComponent( materials );

	geometry->local().setTranslate( x, y, z );

	return geometry;	
}

NodePtr buildEarth( float x, float y, float z )
{
	PrimitivePtr primitive( new SpherePrimitive( 1.0f, VertexFormat::VF_P3_N3_UV2 ) );
	GeometryPtr geometry( new Geometry() );
	geometry->attachPrimitive( primitive );

	MaterialPtr material( new Material() );
	ImagePtr image( new ImageTGA( FileSystem::getInstance().pathForResource( "earth-color.tga" ) ) );
	TexturePtr texture( new Texture( image ) );
	material->setColorMap( texture );
	ImagePtr specularImage( new ImageTGA( FileSystem::getInstance().pathForResource( "earth-specular.tga" ) ) );
	TexturePtr specularMap( new Texture( specularImage ) );
	material->setSpecularMap( specularMap );
	
	MaterialComponentPtr materials( new MaterialComponent() );
	materials->attachMaterial( material );
	geometry->attachComponent( materials );

	NodeComponentPtr rotation( new RotationComponent( Vector3f( 0.0f, 1.0f, 0.0f ), 0.01 ) );
	geometry->attachComponent( rotation );

	geometry->local().setTranslate( x, y, z );

	return geometry;
}

NodePtr buildAtmosphere( float x, float y, float z )
{
	PrimitivePtr primitive( new SpherePrimitive( 1.02f, VertexFormat::VF_P3_N3_UV2 ) );
	GeometryPtr geometry( new Geometry() );
	geometry->attachPrimitive( primitive );

	MaterialPtr material( new Material() );
	ImagePtr image( new ImageTGA( FileSystem::getInstance().pathForResource( "earth-atmosphere.tga" ) ) );
	TexturePtr texture( new Texture( image ) );
	material->setColorMap( texture );
	material->getDepthState()->setEnabled( false );
	material->getAlphaState()->setEnabled( true );
	material->getAlphaState()->setSrcBlendFunc( AlphaState::SrcBlendFunc::SRC_COLOR );
	material->getAlphaState()->setDstBlendFunc( AlphaState::DstBlendFunc::ONE_MINUS_SRC_COLOR );
	geometry->getComponent< MaterialComponent >()->attachMaterial( material );

	NodeComponentPtr rotation( new RotationComponent( Vector3f( 0.0f, 1.0f, 0.0f ), 0.015 ) );
	geometry->attachComponent( rotation );

	geometry->local().setTranslate( x, y, z );

	return geometry;
}

int main( int argc, char **argv )
{
	SimulationPtr sim( new GLSimulation( "Textures", argc, argv ) );

	GroupPtr scene( new Group() );
	scene->attachNode( buildBackground( 0, 0, -5 ) );
	scene->attachNode( buildEarth( 0.5, 0, 0 ) );
	//scene->attachNode( buildAtmosphere( 0.5, 0, 0 ) );

	LightPtr light( new Light() );
	light->local().setTranslate( -2.0f, 1.0f, 4.0f );
	scene->attachNode( light );

	CameraPtr camera( new Camera() );
	camera->local().setTranslate( 0.0f, 0.0f, 4.0f );
	scene->attachNode( camera );

	sim->attachScene( scene );
	return sim->run();
}

