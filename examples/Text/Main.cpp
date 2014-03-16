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

const char *outline_vs = { CRIMILD_TO_STRING(
	in vec3 aPosition;
	in vec2 aTextureCoord;

	uniform mat4 uPMatrix; 
	uniform mat4 uVMatrix; 
	uniform mat4 uMMatrix;

	out vec2 vTextureCoord;

	void main()
	{
		vTextureCoord = aTextureCoord;
		gl_Position = uPMatrix * uVMatrix * uMMatrix * vec4(aPosition, 1.0); 
	}
)};

const char *outline_fs = { CRIMILD_TO_STRING( 
	struct Material {
	    vec4 ambient;
	    vec4 diffuse;
	    vec4 specular;
	    float shininess;
	};

	in vec2 vTextureCoord;

	uniform sampler2D uColorMap;
	uniform Material uMaterial; 

	out vec4 vFragColor;

	const float smoothing = 1.0 / 64.0;

	void main( void ) 
	{ 
		vec4 color = uMaterial.diffuse;
		float distance = texture( uColorMap, vTextureCoord ).r;
		if ( distance > 0.6 ) {
			distance = 0.0f;
		}
    	float alpha = smoothstep( 0.5 - smoothing, 0.5 + smoothing, distance );
    	vFragColor = vec4( color.rgb, alpha );		
	}
)};

class OutlineShaderProgram : public ShaderProgram {
public:
	OutlineShaderProgram( void )
		: ShaderProgram( gl3::Utils::getVertexShaderInstance( outline_vs ), gl3::Utils::getFragmentShaderInstance( outline_fs ) )
	{ 
		registerStandardLocation( ShaderLocation::Type::ATTRIBUTE, ShaderProgram::StandardLocation::POSITION_ATTRIBUTE, "aPosition" );
		registerStandardLocation( ShaderLocation::Type::ATTRIBUTE, ShaderProgram::StandardLocation::TEXTURE_COORD_ATTRIBUTE, "aTextureCoord" );

		registerStandardLocation( ShaderLocation::Type::UNIFORM, ShaderProgram::StandardLocation::PROJECTION_MATRIX_UNIFORM, "uPMatrix" );
		registerStandardLocation( ShaderLocation::Type::UNIFORM, ShaderProgram::StandardLocation::VIEW_MATRIX_UNIFORM, "uVMatrix" );
		registerStandardLocation( ShaderLocation::Type::UNIFORM, ShaderProgram::StandardLocation::MODEL_MATRIX_UNIFORM, "uMMatrix" );

		registerStandardLocation( ShaderLocation::Type::UNIFORM, ShaderProgram::StandardLocation::MATERIAL_COLOR_MAP_UNIFORM, "uColorMap" );
		registerStandardLocation( ShaderLocation::Type::UNIFORM, ShaderProgram::StandardLocation::MATERIAL_AMBIENT_UNIFORM, "uMaterial.ambient" );
		registerStandardLocation( ShaderLocation::Type::UNIFORM, ShaderProgram::StandardLocation::MATERIAL_DIFFUSE_UNIFORM, "uMaterial.diffuse" );
		registerStandardLocation( ShaderLocation::Type::UNIFORM, ShaderProgram::StandardLocation::MATERIAL_SPECULAR_UNIFORM, "uMaterial.specular" );
		registerStandardLocation( ShaderLocation::Type::UNIFORM, ShaderProgram::StandardLocation::MATERIAL_SHININESS_UNIFORM, "uMaterial.shininess" );
	}

	virtual ~OutlineShaderProgram( void )
	{

	}
};

Pointer< Node > generateText( std::string fontName, std::string str, const Vector3f &position, const RGBAColorf &color, ShaderProgram *program ) 
{
	Pointer< Font > font( new Font( FileSystem::getInstance().pathForResource( fontName + ( program != nullptr ? "_sdf" : "" ) + ".tga" ), FileSystem::getInstance().pathForResource( fontName + ".txt" ) ) );
	
	Pointer< Text > text( new Text() );
	text->setFont( font.get() );
	text->setSize( 1.0f );
	text->setText( str );

	if ( program != nullptr ) {
		text->getMaterial()->setProgram( program );
	}
	
	text->getMaterial()->setDiffuse( color );

	text->local().setTranslate( position );

	return text;
}

int main( int argc, char **argv )
{
	Pointer< Simulation > sim( new GLSimulation( "Rendering text", argc, argv ) );

	Pointer< Group > scene( new Group() );

	Pointer< ShaderProgram > sdfProgram( new gl3::SignedDistanceFieldShaderProgram() );
	Pointer< ShaderProgram > outlineProgram( new OutlineShaderProgram() );

	Pointer< Group > texts( new Group() );
	Pointer< Node > text1 = generateText( "LucidaGrande", "Normal text", Vector3f( 0.0f, 0.5f, 0.0f ), RGBAColorf( 1.0f, 0.0f, 0.0f, 1.0f ), nullptr );
	texts->attachNode( text1.get() );
	Pointer< Node > text2 = generateText( "LucidaGrande", "Text with SDF", Vector3f( 0.0f, -0.5f, 0.0f ), RGBAColorf( 0.0f, 1.0f, 0.0f, 1.0f ), sdfProgram.get() );
	texts->attachNode( text2.get() );
	Pointer< Node > text3 = generateText( "LucidaGrande", "Outlined Text", Vector3f( 0.0f, -1.5f, 0.0f ), RGBAColorf( 1.0f, 1.0f, 0.0f, 1.0f ), outlineProgram.get() );
	texts->attachNode( text3.get() );

	scene->attachNode( texts.get() );
	Pointer< NodeComponent > translate( new LambdaComponent( [&]( Node *node, const Time & ) {
		if ( InputState::getCurrentState().isKeyStillDown( 'W' ) ) {
			node->local().translate() += Vector3f( 0.0f, 0.0f, 0.5f );
		}
		else if ( InputState::getCurrentState().isKeyStillDown( 'S' ) ) {
			node->local().translate() -= Vector3f( 0.0f, 0.0f, 0.5f );
		}
	}));
	texts->attachComponent( translate.get() );

	scene->perform( UpdateWorldState() );

	Pointer< Camera > camera( new Camera() );
	camera->local().setTranslate( scene->getWorldBound()->getCenter() + Vector3f( 0.0f, 0.0f, 2.0f ) );
	scene->attachNode( camera.get() );

	sim->setScene( scene.get() );
	return sim->run();
}

