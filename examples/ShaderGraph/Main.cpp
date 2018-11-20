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
#include <Crimild_OpenGL.hpp>
#include <Crimild_SDL.hpp>
#include <Crimild_Import.hpp>

#include "Foundation/Containers/List.hpp"

#include "Rendering/ShaderGraph/ShaderGraph.hpp"
#include "Rendering/ShaderGraph/Nodes/VertexShaderInputs.hpp"
#include "Rendering/ShaderGraph/Nodes/VertexShaderOutputs.hpp"
#include "Rendering/ShaderGraph/Nodes/VertexOutput.hpp"
#include "Rendering/ShaderGraph/Nodes/FragmentInput.hpp"
#include "Rendering/ShaderGraph/Nodes/FragmentColorOutput.hpp"
#include "Rendering/ShaderGraph/Nodes/Multiply.hpp"
#include "Rendering/ShaderGraph/Nodes/Dot.hpp"
#include "Rendering/ShaderGraph/Nodes/Max.hpp"
#include "Rendering/ShaderGraph/Nodes/Subtract.hpp"
#include "Rendering/ShaderGraph/Nodes/Negate.hpp"
#include "Rendering/ShaderGraph/Nodes/Normalize.hpp"
#include "Rendering/ShaderGraph/Nodes/Vector.hpp"
#include "Rendering/ShaderGraph/Nodes/ViewVector.hpp"
#include "Rendering/ShaderGraph/Nodes/WorldNormal.hpp"
#include "Rendering/ShaderGraph/Nodes/Scalar.hpp"
#include "Rendering/ShaderGraph/Nodes/Pow.hpp"
#include "Rendering/ShaderGraph/Nodes/Copy.hpp"

using namespace crimild;
using namespace crimild::import;
using namespace crimild::shadergraph;
using namespace crimild::sdl;

class CustomShader : public ShaderProgram {
public:
	CustomShader( void )
	{
		setVertexShader( crimild::alloc< VertexShader >( createVertexShaderGraph()->build() ) );
		setFragmentShader( crimild::alloc< FragmentShader >( createFragmentShaderGraph()->build() ) );

		registerStandardLocation( ShaderLocation::Type::ATTRIBUTE, ShaderProgram::StandardLocation::POSITION_ATTRIBUTE, "aPosition" );
		registerStandardLocation( ShaderLocation::Type::ATTRIBUTE, ShaderProgram::StandardLocation::NORMAL_ATTRIBUTE, "aNormal" );
		
		registerStandardLocation( ShaderLocation::Type::UNIFORM, ShaderProgram::StandardLocation::PROJECTION_MATRIX_UNIFORM, "uPMatrix" );
		registerStandardLocation( ShaderLocation::Type::UNIFORM, ShaderProgram::StandardLocation::VIEW_MATRIX_UNIFORM, "uVMatrix" );
		registerStandardLocation( ShaderLocation::Type::UNIFORM, ShaderProgram::StandardLocation::MODEL_MATRIX_UNIFORM, "uMMatrix" );
	}

	virtual ~CustomShader( void )
	{
		
	}

private:
	SharedPointer< ShaderGraph > createVertexShaderGraph( void )
	{
		auto graph = Renderer::getInstance()->createShaderGraph();
		
		auto vsInputs = graph->addInputNode< StandardVertexInputs >();
		
		auto worldNormal = graph->addNode< WorldNormal >(
			vsInputs->getModelMatrixUniform(),
			vsInputs->getNormalAttribute()
		)->getResult();
		
		auto viewVector = graph->addNode< ViewVector >(
			vsInputs->getViewPosition()
		)->getResult();

		graph->addOutputNode< StandardVertexOutputs >( vsInputs->getProjectedPosition() );
		graph->addOutputNode< VertexOutput >( "vWorldNormal", worldNormal );
		graph->addOutputNode< VertexOutput >( "vViewVector", viewVector );
		
		return graph;
	}
	
	SharedPointer< ShaderGraph > createFragmentShaderGraph( void )
	{
		auto graph = Renderer::getInstance()->createShaderGraph();
		
		auto worldNormal = graph->addInputNode< FragmentInput >( Variable::Type::VECTOR_3, "vWorldNormal" )->getInput();
		auto viewVector = graph->addInputNode< FragmentInput >( Variable::Type::VECTOR_3, "vViewVector" )->getInput();
		
		auto kONE = graph->addNode< ScalarConstant >( 1.0f, "kONE" )->getVariable();
		auto kZERO = graph->addNode< ScalarConstant >( 0.0f, "kZERO" )->getVariable();
		auto kFresnelExp = graph->addNode< ScalarConstant >( 2.0f, "kFresnelExp" )->getVariable();
		auto kDiffuse = graph->addNode< VectorConstant >( Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ) )->getVector();
		
		// approximate a fresnel effect
		auto effect = graph->addNode< Pow >(
			// 1 - x
			graph->addNode< Subtract >(
				kONE,
				// remap >= 0
				graph->addNode< Max >(
					kZERO,
					// dot( N, D )
					graph->addNode< Dot >(
						worldNormal,
						viewVector
					)->getResult()
				)->getResult()
			)->getResult(),
			kFresnelExp
		)->getResult();
		
		auto rgb = graph->addNode< VectorToScalars >(
			graph->addNode< Multiply >(
				effect,
				kDiffuse
			)->getResult()
		);
		
		// for alpha to 1
		auto finalColor = graph->addNode< ScalarsToVector >(
			rgb->getX(),
			rgb->getY(),
			rgb->getZ(),
			kONE
		)->getVector();
		
		graph->addOutputNode< FragmentColorOutput >( finalColor );
		
		return graph;
	}

};

int main( int argc, char **argv )
{
	auto sim = crimild::alloc< SDLSimulation >( "Shader Graph", crimild::alloc< Settings >( argc, argv ) );

    auto scene = crimild::alloc< Group >();

	auto program = crimild::alloc< CustomShader >();
	if ( program != nullptr ) {
		std::cout << "VERTEX SHADER:\n" << program->getVertexShader()->getSource() << "\n";
		std::cout << "FRAGMENT SHADER:\n" << program->getFragmentShader()->getSource() << "\n";
	}

	auto model = []( const Vector3f &position, SharedPointer< ShaderProgram > const &program ) -> SharedPointer< Node > {
		auto model = SceneImporter::importScene( FileSystem::getInstance().pathForResource( "assets/models/monkey.obj" ) );
		if ( model == nullptr ) {
			return nullptr;
		}

		model->perform( UpdateWorldState() );
		model->local().setScale( 1.0f / model->getWorldBound()->getRadius() );
		auto speed = Random::generate( -0.2f, 0.2f );
		model->attachComponent< RotationComponent >( Vector3f( 0.0f, 1.0f, 0.0f ), speed * Numericf::HALF_PI );

		Material *material = nullptr;
		model->perform( Apply( [ &material ]( crimild::Node *node ) {
			if ( auto ms = node->getComponent< MaterialComponent >() ) {
				if ( auto m = ms->first() ) {
					material = m;
				}
			}
		}));
		
		material->setProgram( program );

		model->local().setTranslate( position );
		
		return model;
	};
	
	scene->attachNode( model( Vector3f( 0.0f, 0.0f, 0.0f ), program ) );
	scene->attachNode( model( Vector3f( -3.0f, 2.0f, -6.0f ), program ) );
	scene->attachNode( model( Vector3f( -3.0f, -1.5f, -4.0f ), program ) );
	scene->attachNode( model( Vector3f( 3.0f, 2.0f, -5.5f ), program ) );
	scene->attachNode( model( Vector3f( 2.5f, -1.5f, -3.5f ), program ) );

    auto camera = crimild::alloc< Camera >();
    camera->local().setTranslate( Vector3f( 0.0f, 0.0f, 2.0f ) );
    scene->attachNode( camera );

    sim->setScene( scene );
	return sim->run();
}

