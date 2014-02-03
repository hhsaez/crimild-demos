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

Pointer< Node > buildLight( const Quaternion4f &rotation, const RGBAColorf &color, float major, float minor, float speed )
{
	Pointer< Group > orbitingLight( new Group() );

	Pointer< SpherePrimitive > primitive( new SpherePrimitive( 0.05f ) );
	Pointer< Geometry > geometry( new Geometry() );
	geometry->attachPrimitive( primitive.get() );
	Pointer< Material > material( new gl3::FlatMaterial( color ) );
	geometry->getComponent< MaterialComponent >()->attachMaterial( material.get() );
	orbitingLight->attachNode( geometry.get() );

	Pointer< Light > light( new Light() );
	light->setColor( color );
	orbitingLight->attachNode( light.get() );

	Pointer< OrbitComponent > orbitComponent( new OrbitComponent( 0.0f, 0.0f, major, minor, speed ) );
	orbitingLight->attachComponent( orbitComponent.get() );

	Pointer< Group > group( new Group() );
	group->attachNode( orbitingLight.get() );
	group->local().setRotate( rotation );

	return group;
}

int main( int argc, char **argv )
{
	Pointer< Simulation > sim( new GLSimulation( "Lighting", argc, argv ) );

	std::cout << "Press 1 to use Phong shading (default)\n"
		  	  << "Press 2 to use Gouraud shading" << std::endl;

	Pointer< Geometry > trefoilKnot( new Geometry() );
	Pointer< Primitive > trefoilKnotPrimitive( new TrefoilKnotPrimitive( Primitive::Type::TRIANGLES, 1.0, VertexFormat::VF_P3_N3 ) );
	trefoilKnot->attachPrimitive( trefoilKnotPrimitive.get() );
	
	Pointer< Material > phongMaterial( new gl3::PhongMaterial() );
	phongMaterial->setAmbient( RGBAColorf( 0.0f, 0.0f, 0.0f, 1.0f ) );
	phongMaterial->setDiffuse( RGBAColorf( 1.0f, 1.0f, 1.0f, 1.0f ) );

	Pointer< Material > gouraudMaterial( new gl3::GouraudMaterial() );
	gouraudMaterial->setAmbient( RGBAColorf( 0.0f, 0.0f, 0.0f, 1.0f ) );
	gouraudMaterial->setDiffuse( RGBAColorf( 1.0f, 1.0f, 1.0f, 1.0f ) );

	MaterialComponent *materials = trefoilKnot->getComponent< MaterialComponent >();
	materials->attachMaterial( gouraudMaterial.get() );

	Pointer< NodeComponent > changeMaterial( new LambdaComponent( [&]( Node *node, const Time & ) {
		if ( InputState::getCurrentState().isKeyDown( '1' ) ) {
			materials->detachAllMaterials();
			materials->attachMaterial( phongMaterial.get() );
			node->perform( UpdateRenderState() );
		}
		else if ( InputState::getCurrentState().isKeyDown( '2' ) ) {
			materials->detachAllMaterials();
			materials->attachMaterial( gouraudMaterial.get() );
			node->perform( UpdateRenderState() );
		}
	}));
	trefoilKnot->attachComponent( changeMaterial.get() );
	
	Pointer< Group > scene( new Group() );
	scene->attachNode( trefoilKnot.get() );

	scene->attachNode( buildLight( 
		Quaternion4f::createFromAxisAngle( Vector3f( 0.0f, 1.0f, 1.0 ).getNormalized(), Numericf::HALF_PI ), 
		RGBAColorf( 1.0f, 0.0f, 0.0f, 1.0f ), 
		-1.0f, 1.25f, 0.95f ).get() );

	scene->attachNode( buildLight( 
		Quaternion4f::createFromAxisAngle( Vector3f( 0.0f, 1.0f, -1.0 ).getNormalized(), -Numericf::HALF_PI ), 
		RGBAColorf( 0.0f, 1.0f, 0.0f, 1.0f ), 
		1.0f, 1.25f, -0.75f ).get() );

	scene->attachNode( buildLight( 
		Quaternion4f::createFromAxisAngle( Vector3f( 0.0f, 1.0f, 0.0 ).getNormalized(), Numericf::HALF_PI ), 
		RGBAColorf( 0.0f, 0.0f, 1.0f, 1.0f ), 
		1.25f, 1.0f, -0.85f ).get() );

	Pointer< Camera > camera( new Camera() );
	camera->local().setTranslate( 0.0f, 0.0f, 3.0f );
	scene->attachNode( camera.get() );

	sim->setScene( scene.get() );
	return sim->run();
}

