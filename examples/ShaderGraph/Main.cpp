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
#include <Crimild_GLFW.hpp>
#include <Crimild_Import.hpp>

#include "Foundation/Containers/List.hpp"

#include "Rendering/ShaderGraph/ShaderGraph.hpp"
#include "Rendering/ShaderGraph/Nodes/VertexShaderInput.hpp"
#include "Rendering/ShaderGraph/Nodes/VertexShaderOutput.hpp"
#include "Rendering/ShaderGraph/Nodes/FragmentShaderInput.hpp"
#include "Rendering/ShaderGraph/Nodes/FragmentShaderOutput.hpp"
#include "Rendering/ShaderGraph/Nodes/Multiply.hpp"
#include "Rendering/ShaderGraph/Nodes/Dot.hpp"
#include "Rendering/ShaderGraph/Nodes/Vector.hpp"
#include "Rendering/ShaderGraph/Nodes/Scalar.hpp"
#include "Rendering/ShaderGraph/ShaderBuilder.hpp"
#include "Rendering/ShaderGraph/OpenGLShaderBuilder.hpp"

using namespace crimild;
using namespace crimild::import;
using namespace crimild::shadergraph;
using namespace crimild::shadergraph::nodes;

SharedPointer< ShaderGraph > createVertexShaderGraph( void )
{
	// vertex shader graph	
	auto vsGraph = crimild::alloc< ShaderGraph >();	
	auto vsInputs = vsGraph->createNode< nodes::VertexShaderInput >();

	// constants
	auto kZero = vsGraph->createNode< nodes::Scalar >( 0.0f );
	auto kOne = vsGraph->createNode< nodes::Scalar >( 1.0f );
	auto kDirection = vsGraph->createNode< nodes::Vector >( Vector4f( 0.0f, 0.0f, -1.0f, 0.0f ) );

	// compute model-space position
	auto modelPosition = vsGraph->createNode< nodes::Vector >();
	vsGraph->connect( vsInputs->getPosition(), modelPosition->getInputXYZ() );
	vsGraph->connect( kOne->getOutputValue(), modelPosition->getInputW() );

	// compute world-space position
	auto worldPosition= vsGraph->createNode< nodes::Multiply >( Outlet::Type::VECTOR_4 );
	vsGraph->connect( vsInputs->getMMatrix(), worldPosition->getA() );
	vsGraph->connect( modelPosition->getOutputXYZW(), worldPosition->getB() );

	// compute view-space position
	auto viewPosition = vsGraph->createNode< nodes::Multiply >( Outlet::Type::VECTOR_4 );
	vsGraph->connect( vsInputs->getVMatrix(), viewPosition->getA() );
	vsGraph->connect( worldPosition->getOutput(), viewPosition->getB() );

	// compute screen-space position
	auto screenPosition = vsGraph->createNode< nodes::Multiply >( Outlet::Type::VECTOR_4 );
	vsGraph->connect( vsInputs->getPMatrix(), screenPosition->getA() );
	vsGraph->connect( viewPosition->getOutput(), screenPosition->getB() );

	// compute model-space normal
	auto modelNormal = vsGraph->createNode< nodes::Vector >();
	vsGraph->connect( vsInputs->getNormal(), modelNormal->getInputXYZ() );
	vsGraph->connect( kZero->getOutputValue(), modelNormal->getInputW() );

	// compute world-space normal
	auto worldNormal = vsGraph->createNode< nodes::Multiply >( Outlet::Type::VECTOR_4 );
	vsGraph->connect( vsInputs->getMMatrix(), worldNormal->getA() );
	vsGraph->connect( modelNormal->getOutputXYZW(), worldNormal->getB() );
	auto worldNormalXYZ = vsGraph->createNode< nodes::Vector >();
	vsGraph->connect( worldNormal->getOutput(), worldNormalXYZ->getInputXYZW() );
	
	// connect outputs
	auto vsOutputs = vsGraph->createNode< nodes::VertexShaderOutput >();
	vsGraph->connect( screenPosition->getOutput(), vsOutputs->getScreenPosition() );
    vsGraph->connect( worldNormalXYZ->getOutputXYZ(), vsOutputs->getWorldNormal() );

	return vsGraph;
}

SharedPointer< ShaderGraph > createFragmentShaderGraph( void )
{
	// fragment shader graph
	auto fsGraph = crimild::alloc< ShaderGraph >();
	auto fsInput = fsGraph->createNode< FragmentShaderInput >();
	auto fsOutput = fsGraph->createNode< FragmentShaderOutput >();

	// constants
	auto kAlpha = fsGraph->createNode< nodes::Scalar >( 1.0f );

	// frag color
	auto fragColor = fsGraph->createNode< nodes::Vector >();
	fsGraph->connect( fsInput->getWorldNormal(), fragColor->getInputXYZ() );
	fsGraph->connect( kAlpha->getOutputValue(), fragColor->getInputW() );

	// connect outputs
    fsGraph->connect( fragColor->getOutputXYZW(), fsOutput->getFragColor() );

	return fsGraph;
}

int main( int argc, char **argv )
{
	auto sim = crimild::alloc< GLSimulation >( "Shader Graph", crimild::alloc< Settings >( argc, argv ) );
	sim->getRenderer()->getScreenBuffer()->setClearColor( RGBAColorf( 0.5f, 0.5f, 0.5f, 1.0f ) );

    auto scene = crimild::alloc< Group >();

	auto model = SceneImporter::importScene( FileSystem::getInstance().pathForResource( "assets/models/monkey.obj" ) );
	if ( model == nullptr ) {
		return -1;
	}
	
	model->perform( UpdateWorldState() );
	model->local().setScale( 1.0f / model->getWorldBound()->getRadius() );
    //model->attachComponent< RotationComponent >( Vector3f( 0.0f, 1.0f, 0.0f ), 0.25f * Numericf::HALF_PI );
	scene->attachNode( model );

	Material *material = nullptr;
	model->perform( Apply( [ &material ]( crimild::Node *node ) {
		if ( auto ms = node->getComponent< MaterialComponent >() ) {
			if ( auto m = ms->first() ) {
				material = m;
			}
		}
	}));

	auto vsGraph = createVertexShaderGraph();
	auto fsGraph = createFragmentShaderGraph();
	auto shaderBuilder = crimild::alloc< OpenGLShaderBuilder >();
	auto program = shaderBuilder->build( vsGraph, fsGraph );
	
	material->setProgram( program );

    auto camera = crimild::alloc< Camera >();
    camera->local().setTranslate( Vector3f( 0.0f, 0.0f, 3.0f ) );
    scene->attachNode( camera );

    sim->setScene( scene );
	return sim->run();
}

