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

#include <fstream>
#include <string>
#include <vector>

using namespace crimild;

class DebugRenderPass : public RenderPass {
public:
	DebugRenderPass( RenderPassPtr actualRenderPass )
		: _actualRenderPass( actualRenderPass ),
		  _debugMaterial( new Material() )
	{

	}

	virtual ~DebugRenderPass( void )
	{

	}

	virtual void render( Renderer *renderer, VisibilitySet *vs, Camera *camera ) 
	{
		if ( _actualRenderPass != nullptr ) {
			_actualRenderPass->render( renderer, vs, camera );
		}

		vs->foreachGeometry( [&]( Geometry *geometry ) {
			updateDebugPrimitive( geometry );
			if ( _debugPrimitive != nullptr ) { 
				RenderPass::render( renderer, geometry, _debugPrimitive.get(), _debugMaterial.get(), camera );
			}
		});
	}

private:
	void updateDebugPrimitive( Geometry *geometry )
	{
		std::vector< float > vertices;

		geometry->foreachPrimitive( [&]( PrimitivePtr primitive ) {
			VertexBufferObject *vbo = primitive->getVertexBuffer();
			const VertexFormat &vf = vbo->getVertexFormat();

			for ( int i = 0; i < vbo->getVertexCount(); i++ ) {
				Vector3f pos = vbo->getPositionAt( i );

				// render normals
				Vector3f normal = vbo->getNormalAt( i );
				vertices.push_back( pos[ 0 ] ); vertices.push_back( pos[ 1 ] ); vertices.push_back( pos[ 2 ] );
				vertices.push_back( 1.0f ); vertices.push_back( 1.0f ); vertices.push_back( 1.0f ); vertices.push_back( 1.0f );
				vertices.push_back( pos[ 0 ] + 0.05 * normal[ 0 ] ); vertices.push_back( pos[ 1 ] + 0.05 * normal[ 1 ] ); vertices.push_back( pos[ 2 ] + 0.05 * normal[ 2 ] );
				vertices.push_back( 0.0f ); vertices.push_back( 1.0f ); vertices.push_back( 0.0f ); vertices.push_back( 1.0f );

				if ( vf.hasTangents() ) {
					Vector3f tangent = vbo->getTangentAt( i );
					vertices.push_back( pos[ 0 ] ); vertices.push_back( pos[ 1 ] ); vertices.push_back( pos[ 2 ] );
					vertices.push_back( 1.0f ); vertices.push_back( 1.0f ); vertices.push_back( 1.0f ); vertices.push_back( 1.0f );
					vertices.push_back( pos[ 0 ] + 0.05 * tangent[ 0 ] ); vertices.push_back( pos[ 1 ] + 0.05 * tangent[ 1 ] ); vertices.push_back( pos[ 2 ] + 0.05 * tangent[ 2 ] );
					vertices.push_back( 1.0f ); vertices.push_back( 0.0f ); vertices.push_back( 1.0f ); vertices.push_back( 1.0f );
				}
			}
		});

		VertexFormat format = VertexFormat::VF_P3_C4;

		int vertexCount = vertices.size() / format.getVertexSize();
		std::vector< unsigned short > indices( vertexCount );
		for ( int i = 0; i < vertexCount; i++ ) {
			indices.push_back( i );
		}

		VertexBufferObjectPtr vbo( new VertexBufferObject( format, vertexCount, &vertices[ 0 ] ) );
		IndexBufferObjectPtr ibo( new IndexBufferObject( indices.size(), &indices[ 0 ] ) );
		_debugPrimitive = PrimitivePtr( new Primitive( Primitive::Type::LINES ) );
		_debugPrimitive->setVertexBuffer( vbo );
		_debugPrimitive->setIndexBuffer( ibo );
	}

private:
	RenderPassPtr _actualRenderPass;
	PrimitivePtr _debugPrimitive;
	MaterialPtr _debugMaterial;
};

typedef std::shared_ptr< DebugRenderPass > DebugRenderPassPtr;

int main( int argc, char **argv )
{
	SimulationPtr sim( new GLSimulation( "IronMan", argc, argv ) );

	GroupPtr scene( new Group() );

	OBJLoader loader( FileSystem::getInstance().pathForResource( "ironman/Iron_Man.obj" ) );
	NodePtr ironman = loader.load();
	if ( ironman != nullptr ) {
		RotationComponentPtr rotationComponent( new RotationComponent( Vector3f( 0, 1, 0 ), 0.01 ) );
		ironman->attachComponent( rotationComponent );
		scene->attachNode( ironman );
	}

	LightPtr light( new Light() );
	light->local().setTranslate( 1.0f, 2.0f, 5.0f );
	scene->attachNode( light );

	CameraPtr camera( new Camera() );
	camera->local().setTranslate( 0.0f, 3.25f, 1.5f );
	scene->attachNode( camera );

	OffscreenRenderPassPtr renderPass( new OffscreenRenderPass() );
	// RenderPassPtr renderPass( new RenderPass() );
	// DebugRenderPassPtr debugRenderPass( new DebugRenderPass( renderPass ) );
	// camera->setRenderPass( debugRenderPass );
	camera->setRenderPass( renderPass );
	ImageEffectPtr glowEffect( new ImageEffect() );
	ShaderProgramPtr glowProgram( new gl3::GlowShaderProgram() );
	glowEffect->setProgram( glowProgram );
	renderPass->attachImageEffect( glowEffect );

	sim->attachScene( scene );
	return sim->run();
}

