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

Pointer< Geometry > buildSkin( void )
{
	int vertexCount = 12;
	float vertices[] = {
		// front
		-1.0f, +1.0f, 0.0f, 0.0f, 0.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		+1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		+1.0f, +1.0f, 0.0f, 0.0f, 0.0f,

		// center
		-1.0f, +1.0f, 2.0f, 1.0f, 0.0f,
		-1.0f, -1.0f, 2.0f, 1.0f, 0.0f,
		+1.0f, -1.0f, 2.0f, 1.0f, 0.0f,
		+1.0f, +1.0f, 2.0f, 1.0f, 0.0f,

		// back
		-1.0f, +1.0f, 4.0f, 2.0f, 0.0f,
		-1.0f, -1.0f, 4.0f, 2.0f, 0.0f,
		+1.0f, -1.0f, 4.0f, 2.0f, 0.0f,
		+1.0f, +1.0f, 4.0f, 2.0f, 0.0f,
	};

	int indexCount = 48;
	unsigned short indices[] = {
		// right
		3, 2, 6, 3, 6, 7,
		7, 6, 10, 7, 10, 11,

		// top
		0, 3, 7, 0, 7, 4,
		4, 7, 11, 4, 11, 8,

		// left
		8, 9, 5, 8, 5, 4,
		4, 5, 1, 4, 1, 0,

		// bottom
		9, 10, 6, 9, 6, 5,
		5, 6, 2, 5, 2, 1
	};

	Pointer< VertexBufferObject > vbo( new VertexBufferObject( VertexFormat::VF_P3_UV2, vertexCount, vertices ) );
	Pointer< IndexBufferObject > ibo( new IndexBufferObject( indexCount, indices ) );
	
	Pointer< Primitive > primitive( new Primitive( Primitive::Type::TRIANGLES ) );
	primitive->setVertexBuffer( vbo.get() );
	primitive->setIndexBuffer( ibo.get() );

	Pointer< Geometry > geometry( new Geometry() );
	geometry->attachPrimitive( primitive.get() );

	return geometry;
}

Pointer< Group > buildJoint( std::string name, unsigned int index, char rotKey, char invRotKey ) 
{
	Pointer< Group > joint( new Group( name ) );

	Pointer< NodeComponent > jointComponent( new JointComponent() );
	joint->attachComponent( jointComponent.get() );

	Pointer< LambdaComponent > jointControls( new LambdaComponent( [=]( crimild::Node *node, const Time &t ) {
		if ( InputState::getCurrentState().isKeyStillDown( rotKey ) ) {
			node->local().rotate() *= Quaternion4f::createFromAxisAngle( Vector3f( 1.0f, 0.0f, 0.0f ), t.getDeltaTime() );
		}
		if ( InputState::getCurrentState().isKeyStillDown( invRotKey ) ) {
			node->local().rotate() *= Quaternion4f::createFromAxisAngle( Vector3f( 1.0f, 0.0f, 0.0f ), -t.getDeltaTime() );
		}
	}));
	joint->attachComponent( jointControls.get() );

	return joint;
}

Pointer< Node > buildArm( void )
{
	Pointer< Group > arm( new Group() );

	Pointer< Group > shoulderJoint = buildJoint( "shoulder", 0, '1', '2' );
	arm->attachNode( shoulderJoint.get() );

	Pointer< Group > elbowJoint = buildJoint( "elbow", 1, '3', '4' );
	elbowJoint->local().setTranslate( 0.0f, 0.0f, 2.0f );
	shoulderJoint->attachNode( elbowJoint.get() );

	Pointer< Group > wristJoint = buildJoint( "wrist", 2, '5', '6' );
	wristJoint->local().setTranslate( 0.0f, 0.0f, 2.0f );
	elbowJoint->attachNode( wristJoint.get() );

	shoulderJoint->getComponent< JointComponent >()->computeInverseBindMatrix();
	elbowJoint->getComponent< JointComponent >()->computeInverseBindMatrix();
	wristJoint->getComponent< JointComponent >()->computeInverseBindMatrix();

	Pointer< Geometry > skin = buildSkin();
	arm->attachNode( skin.get() );

	Pointer< SkinComponent > skinning( new SkinComponent() );
	skinning->attachJoint( shoulderJoint.get() );
	skinning->attachJoint( elbowJoint.get() );
	skinning->attachJoint( wristJoint.get() );
	skin->attachComponent( skinning.get() );

	Pointer< Material > material( new Material() );
	Pointer< ShaderProgram > program( new gl3::SkinningShaderProgram() );
	material->setProgram( program.get() );
	skin->getComponent< MaterialComponent >()->attachMaterial( material.get() );

	return arm;
}

int main( int argc, char **argv )
{
	Pointer< Simulation > sim( new GLSimulation( "Skeletal Animation", argc, argv ) );

	Pointer< Group > scene( new Group() );

	Pointer< Node > arm = buildArm();
	arm->local().setTranslate( 0.0f, 0.0f, -2.0f );
	Pointer< LambdaComponent > controls( new LambdaComponent( [=]( crimild::Node *node, const Time &t ) {
		if ( InputState::getCurrentState().isKeyStillDown( 'W' ) ) {
			node->local().translate() += Vector3f( 0.0f, 0.0f, t.getDeltaTime() );
		}
		if ( InputState::getCurrentState().isKeyStillDown( 'S' ) ) {
			node->local().translate() += Vector3f( 0.0f, 0.0f, -t.getDeltaTime() );
		}
		if ( InputState::getCurrentState().isKeyStillDown( 'A' ) ) {
			node->local().rotate() *= Quaternion4f::createFromAxisAngle( Vector3f( 0.0f, 1.0f, 0.0f ), t.getDeltaTime() );
		}
		if ( InputState::getCurrentState().isKeyStillDown( 'D' ) ) {
			node->local().rotate() *= Quaternion4f::createFromAxisAngle( Vector3f( 0.0f, 1.0f, 0.0f ), -t.getDeltaTime() );
		}
	}));
	arm->attachComponent( controls.get() );
	scene->attachNode( arm.get() );

	Pointer< Camera > camera( new Camera() );
	camera->local().setTranslate( 0.0f, 0.0f, 15.0f );

	Pointer< HierarchyRenderPass > renderPass( new HierarchyRenderPass() );
	renderPass->setTargetScene( scene.get() );
	renderPass->setRenderBoundings( true );
	camera->setRenderPass( renderPass.get() );

	scene->attachNode( camera.get() );

	sim->setScene( scene.get() );
	return sim->run();
}

