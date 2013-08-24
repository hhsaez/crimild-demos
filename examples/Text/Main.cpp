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

NodePtr generateText( std::string fontName, std::string str, const Vector3f &position, const RGBAColorf &color, bool useSDF = false ) 
{
	FontPtr font( new Font( FileSystem::getInstance().pathForResource( fontName + ( useSDF ? "_sdf" : "" ) + ".tga" ), FileSystem::getInstance().pathForResource( fontName + ".txt" ) ) );
	
	TextPtr text( new Text() );
	text->setFont( font );
	text->setSize( 1.0f );
	text->setText( str );

	if ( useSDF ) {
		gl3::SignedDistanceFieldShaderProgramPtr program( new gl3::SignedDistanceFieldShaderProgram() );
		text->getMaterial()->setProgram( program );
	}
	
	text->getMaterial()->setDiffuse( color );

	text->local().setTranslate( position );

	return text;
}

int main( int argc, char **argv )
{
	SimulationPtr sim( new GLSimulation( "Rendering text", argc, argv ) );

	GroupPtr scene( new Group() );

	GroupPtr texts( new Group() );
	NodePtr text1 = generateText( "LucidaGrande", "Normal text", Vector3f( 0.0f, 0.5f, 0.0f ), RGBAColorf( 1.0f, 0.0f, 0.0f, 1.0f ), false );
	texts->attachNode( text1 );
	NodePtr text2 = generateText( "LucidaGrande", "Text with SDF", Vector3f( 0.0f, -0.5f, 0.0f ), RGBAColorf( 0.0f, 1.0f, 0.0f, 1.0f ), true );
	texts->attachNode( text2 );

	scene->attachNode( texts );
	NodeComponentPtr translate( new LambdaComponent( [&]( Node *node, const Time & ) {
		if ( InputState::getCurrentState().isKeyStillDown( 'W' ) ) {
			node->local().translate() += Vector3f( 0.0f, 0.0f, 1.0f );
		}
		else if ( InputState::getCurrentState().isKeyStillDown( 'S' ) ) {
			node->local().translate() -= Vector3f( 0.0f, 0.0f, 1.0f );
		}
	}));
	texts->attachComponent( translate );

	scene->perform( UpdateWorldState() );

	CameraPtr camera( new Camera() );
	camera->local().setTranslate( scene->getWorldBound()->getCenter() + Vector3f( 0.0f, 0.0f, 2.0f ) );
	scene->attachNode( camera );

	sim->attachScene( scene );
	return sim->run();
}

