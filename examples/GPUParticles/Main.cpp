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
	Pointer< Simulation > sim( new GLSimulation( "GPU Particle System", argc, argv ) );

	Pointer< Group > scene( new Group() );

	Pointer< Primitive > ballPrimitive( new SpherePrimitive( 2.0f ) );
	Pointer< Geometry > sphere( new Geometry() );
	sphere->attachPrimitive( ballPrimitive.get() );
	Pointer< Material > material( new Material() );
	material->setDiffuse( RGBAColorf( 1.0f, 0.0f, 0.0f, 1.0f ) );
	sphere->getComponent< MaterialComponent >()->attachMaterial( material.get() );
	scene->attachNode( sphere.get() );

	Pointer< gl3::ParticleSystem > particleSystem( new gl3::ParticleSystem() );
	ParticleSystemComponent *particleSystemComponent = particleSystem->getComponent< ParticleSystemComponent >();
	particleSystemComponent->setParticleCount( 500 );
	particleSystemComponent->setParticleSize( 40.0f );
	particleSystemComponent->setParticleDuration( 1.0f );
	particleSystemComponent->setGravity( Vector3f( 0.0f, -9.8f, 0.0f ) );
	particleSystemComponent->setSpread( Vector3f( 2.0f, 1.0f, 2.0f ) );
	particleSystemComponent->setVelocity( Vector3f( 10.0f, 0.0f, 0.0f ) );
	particleSystemComponent->setLooping( true );
	particleSystemComponent->generateParticles();
	Pointer< Image > image( new ImageTGA( FileSystem::getInstance().pathForResource( "particle.tga" ) ) );
	Pointer< Texture > texture( new Texture( image.get() ) );
	particleSystemComponent->getParticleMaterial()->getDepthState()->setEnabled( false );
	particleSystemComponent->getParticleMaterial()->setColorMap( texture.get() );
	particleSystemComponent->setShape( sphere->worldBound() );
	scene->attachNode( particleSystem.get() );

	Pointer< Light > light( new Light() );
	light->local().setTranslate( 0.0f, 0.0f, 10.0f );
	scene->attachNode( light.get() );

	Pointer< Camera > camera( new Camera() );
	camera->local().setTranslate( 5.0f, -5.0f, 25.0f );
	scene->attachNode( camera.get() );

	Pointer< NodeComponent > controls( new LambdaComponent( [&]( Node *node, const Time &t ) {
		if ( InputState::getCurrentState().isKeyStillDown( 'W' ) ) {
			sphere->local().translate() += Vector3f( 0.0f, 4.0f * t.getDeltaTime(), 0.0f );
		}
		if ( InputState::getCurrentState().isKeyStillDown( 'S' ) ) {
			sphere->local().translate() += Vector3f( 0.0f, -4.0f * t.getDeltaTime(), 0.0f );
		}
		if ( InputState::getCurrentState().isKeyStillDown( 'A' ) ) {
			sphere->local().translate() += Vector3f( -4.0f * t.getDeltaTime(), 0.0f, 0.0f );
		}
		if ( InputState::getCurrentState().isKeyStillDown( 'D' ) ) {
			sphere->local().translate() += Vector3f( 4.0f * t.getDeltaTime(), 0.0f, 0.0f );
		}

		if ( InputState::getCurrentState().isKeyStillDown( 'Q' ) ) {
			particleSystem->local().rotate() *= Quaternion4f::createFromAxisAngle( Vector3f( 0.0f, 0.0f, 1.0f ), 0.1f );
		}
		if ( InputState::getCurrentState().isKeyStillDown( 'E' ) ) {
			particleSystem->local().rotate() *= Quaternion4f::createFromAxisAngle( Vector3f( 0.0f, 0.0f, 1.0f ), -0.1f );
		}
	}));
	scene->attachComponent( controls.get() );

	sim->setScene( scene.get() );
	return sim->run();
}

