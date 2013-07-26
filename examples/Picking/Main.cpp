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

class PickingComponent : public NodeComponent {
public:
	PickingComponent( void )
		: NodeComponent( "picking" )
	{

	}

	virtual ~PickingComponent( void )
	{

	}

	virtual void update( const Time &t ) override 
	{
		if ( InputState::getCurrentState().isMouseButtonDown( 0 ) ) {
			Camera *camera = static_cast< Camera * >( getNode() );

			Ray3f ray;
			Vector2f mousePos = InputState::getCurrentState().getNormalizedMousePosition();
			if ( camera->getPickRay( mousePos[ 0 ], mousePos[ 1 ], ray ) ) {
				SelectNodes selectNodes( [&]( Node *node ) {
					MaterialComponent *materials = node->getComponent< MaterialComponent >();
					if ( materials ) {
						if ( node->getWorldBound()->intersects( ray ) ) {
							materials->foreachMaterial( []( MaterialPtr &material ) {
								float r = rand() % 255 / 255.0f;
								float g = rand() % 255 / 255.0f;
								float b = rand() % 255 / 255.0f;
								material->setDiffuse( RGBAColorf( r, g, b, 1.0f ) );
							});
						}
					}

					return false;
				});

				getNode()->getParent()->perform( selectNodes );
			}
		}
	}
};

NodePtr makeSphere( float x, float y, float z )
{
	PrimitivePtr primitive( new ParametricSpherePrimitive( Primitive::Type::TRIANGLES, 1.0f ) );
	GeometryPtr geometry( new Geometry() );
	geometry->attachPrimitive( primitive );

	MaterialPtr material( new Material() );
	material->setDiffuse( RGBAColorf( 0.75f, 0.75f, 0.75f, 1.0f ) );
	MaterialComponentPtr materials( new MaterialComponent() );
	materials->attachMaterial( material );
	geometry->attachComponent( materials );

	geometry->local().setTranslate( x, y, z );

	geometry->setName( "sphere" );

	return geometry;	
}

int main( int argc, char **argv )
{
	SimulationPtr sim( new GLSimulation( "Selecting objects with the mouse", argc, argv ) );

	GroupPtr scene( new Group() );

	GroupPtr spheres( new Group() );
	for ( float x = -5.0f; x <= 5.0f; x++ ) {
		for ( float y = -3.0f; y <= 3.0f; y++ ) {
			spheres->attachNode( makeSphere( x * 3.0f, y * 3.0f, 0.0f ) );
		}
	}
	RotationComponentPtr rotationComponent( new RotationComponent( Vector3f( 0.0f, 1.0f, 0.0f ), 0.01f ) );
	spheres->attachComponent( rotationComponent );
	scene->attachNode( spheres );

	CameraPtr camera( new Camera() );
	NodeComponentPtr pickingComponent( new PickingComponent() );
	camera->attachComponent( pickingComponent );
	camera->local().setTranslate( 10.0f, 15.0f, 50.0f );
	camera->local().setRotate( Vector3f( -1.0f, 0.5f, 0.0f ).getNormalized(), 0.1 * Numericf::PI );
	scene->attachNode( camera );

	sim->attachScene( scene );
	return sim->run();
}

