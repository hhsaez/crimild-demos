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
	std::cout << "Usage: "
			  << "\nPress 1 to toggle color map"
		  	  << "\nPress 2 to toggle specular map"
		  	  << "\nPress 3 to toggle normal map" << std::endl;

	Pointer< Simulation > sim( new GLSimulation( "A simple example", argc, argv ) );

	float vertices[] = {
		-2.0f, +2.0f, 0.0f, 	0.0f, 0.0f, 1.0f,	0.0f, -1.0f, 0.0f,	0.0f, 0.0f,
		-2.0f, -2.0f, 0.0f, 	0.0f, 0.0f, 1.0f,	1.0f, 0.0f, 0.0f, 	0.0f, 1.0f, 
		+2.0f, -2.0f, 0.0, 		0.0f, 0.0f, 1.0f,	0.0f, +1.0f, 0.0f,	1.0f, 1.0f, 
		+2.0f, +2.0f, 0.0f,		0.0f, 0.0f, 1.0f, 	-1.0f, 0.0f, 0.0f, 	1.0f, 0.0f
	};

	unsigned short indices[] = {
		0, 1, 2, 0, 2, 3
	};

	Pointer< VertexBufferObject > vbo( new VertexBufferObject( VertexFormat::VF_P3_N3_TG3_UV2, 4, vertices ) );
	Pointer< IndexBufferObject > ibo( new IndexBufferObject( 6, indices ) );
	
	Pointer< Primitive > primitive( new Primitive( Primitive::Type::TRIANGLES ) );
	primitive->setVertexBuffer( vbo );
	primitive->setIndexBuffer( ibo );

	Pointer< Geometry > geometry( new Geometry() );
	geometry->attachPrimitive( primitive );

	Pointer< Material > material( new Material() );
		Pointer< Image > colorImage( new ImageTGA( FileSystem::getInstance().pathForResource( "stone-color.tga" ) ) );
		Pointer< Texture > colorMap( new Texture( colorImage ) );
		material->setColorMap( colorMap );
		Pointer< Image > specularImage( new ImageTGA( FileSystem::getInstance().pathForResource( "stone-specular.tga" ) ) );
		Pointer< Texture > specularMap( new Texture( specularImage ) );
		material->setSpecularMap( specularMap );
		Pointer< Image > normalImage( new ImageTGA( FileSystem::getInstance().pathForResource( "stone-normal.tga" ) ) );
		Pointer< Texture > normalMap( new Texture( normalImage ) );
		material->setNormalMap( normalMap );
	geometry->getComponent< MaterialComponent >()->attachMaterial( material );
	
	Pointer< Group > scene( new Group() );
	scene->attachNode( geometry );

	Pointer< Group > interactiveLight( new Group() );
		Pointer< SpherePrimitive > lightPrimitive( new SpherePrimitive( 0.025f, VertexFormat::VF_P3 ) );
		Pointer< Geometry > lightGeometry( new Geometry() );
		lightGeometry->attachPrimitive( lightPrimitive );
		interactiveLight->attachNode( lightGeometry );
		Pointer< Light > light( new Light() );
		light->local().setRotate( Vector3f( 0.0f, 1.0f, 0.0f ), -Numericf::HALF_PI );
		interactiveLight->attachNode( light );
		interactiveLight->local().setTranslate( 1.0f, 1.0f, 1.0f );
	scene->attachNode( interactiveLight );

	Pointer< Camera > camera( new Camera() );
	camera->local().setTranslate( 0.0f, 0.0f, 6.0f );
	scene->attachNode( camera );

	Pointer< LambdaComponent > controls( new LambdaComponent( [&]( Node *, const Time &t ) {
		if ( InputState::getCurrentState().isKeyDown( '1' ) ) {
			material->setColorMap( material->getColorMap() != nullptr ? nullptr : colorMap );
		}

		if ( InputState::getCurrentState().isKeyDown( '2' ) ) {
			material->setSpecularMap( material->getSpecularMap() != nullptr ? nullptr : specularMap );
		}
		
		if ( InputState::getCurrentState().isKeyDown( '3' ) ) {
			material->setNormalMap( material->getNormalMap() != nullptr ? nullptr : normalMap );
		}

		if ( InputState::getCurrentState().isKeyStillDown( 'W' ) ) {
			interactiveLight->local().translate() += Vector3f( 0.0f, t.getDeltaTime(), 0.0f );
		}
		if ( InputState::getCurrentState().isKeyStillDown( 'S' ) ) {
			interactiveLight->local().translate() += Vector3f( 0.0f, -t.getDeltaTime(), 0.0f );
		}
		if ( InputState::getCurrentState().isKeyStillDown( 'A' ) ) {
			interactiveLight->local().translate() += Vector3f( -t.getDeltaTime(), 0.0f, 0.0f );
		}
		if ( InputState::getCurrentState().isKeyStillDown( 'D' ) ) {
			interactiveLight->local().translate() += Vector3f( t.getDeltaTime(), 0.0f, 0.0f );
		}
	}));
	scene->attachComponent( controls );

	sim->setScene( scene );
	return sim->run();
}

