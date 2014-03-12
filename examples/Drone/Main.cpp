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
#include <Crimild_AL.hpp>

#include <fstream>
#include <string>
#include <vector>

using namespace crimild;
using namespace crimild::al;

class DroneComponent : public NodeComponent {
public:
	DroneComponent( void ) { }
	virtual ~DroneComponent( void ) { }

	virtual void onAttach( void )
	{
		getNode()->local().setTranslate( 0.0f, 10.0f, -80.0f );
	}

	virtual void update( const Time &t ) override
	{
		float z = -50.0f + 50.0f * std::sin( t.getCurrentTime() );
		float y = 5.0f + 1.0f * std::sin( 2.0f * t.getCurrentTime() );

		getNode()->local().setTranslate( 0.0f, y, z );

		float pitchAngle = -0.15f * std::sin( 0.5f * t.getCurrentTime() ) * std::cos( 0.5f * t.getCurrentTime() );
		float rollAngle = 0.025f * std::sin( 4.0f * t.getCurrentTime() );
		Quaternion4f pitch, roll, yaw;
		pitch.fromAxisAngle( Vector3f( 1.0f, 0.0f, 0.0f ), pitchAngle );
		roll.fromAxisAngle( Vector3f( 0.0f, 0.0f, 1.0f ), rollAngle );
		yaw.fromAxisAngle( Vector3f( 0.0f, 1.0f, 0.0f ), -Numericf::HALF_PI );
		getNode()->local().setRotate( roll * pitch * yaw );
	}
};

Pointer< Node > loadDrone( void )
{
    Pointer< Group > drone( new Group( "drone" ) );
    
	OBJLoader loader( FileSystem::getInstance().pathForResource( "assets/MQ-27b.obj" ) );
	Pointer< Node > droneModel = loader.load();
	if ( droneModel != nullptr ) {
		drone->attachNode( droneModel.get() );
        
		Pointer< NodeComponent > droneComponent( new DroneComponent() );
		drone->attachComponent( droneComponent.get() );
        
		Pointer< AudioClip > audioClip( new WavAudioClip( FileSystem::getInstance().pathForResource( "drone_mono.wav" ) ) );
		Pointer< AudioComponent > audioComponent( new AudioComponent( audioClip.get() ) );
		drone->attachComponent( audioComponent.get() );
		audioComponent->play( true );
	}
    
    return drone;
}

Pointer< Node > makeGround( void )
{
	Pointer< Primitive > primitive( new QuadPrimitive( 200.0f, 200.0f ) );
	Pointer< Geometry > geometry( new Geometry() );
	geometry->attachPrimitive( primitive.get() );
	geometry->local().setRotate( Vector3f( 1.0f, 0.0f, 0.0f ), -Numericf::HALF_PI );
    geometry->local().setTranslate( 0.0f, 0.0f, -30.0f );
	
	return geometry;
}

int main( int argc, char **argv )
{
	Pointer< Simulation > sim( new GLSimulation( "Drone", argc, argv ) );

	Pointer< Group > scene( new Group() );
    scene->attachNode( loadDrone().get() );
	scene->attachNode( makeGround().get() );

	AudioManager::getInstance().setGeneralGain( 80.0f );

	Pointer< Light > light( new Light() );
	light->local().setTranslate( 10.0f, 25.0f, 20.0f );
    light->local().lookAt( Vector3f( 0.0f, 0.0f, -8.0f ), Vector3f( 0.0f, 1.0f, 0.0 ) );
    light->setCastShadows( true );
    light->setShadowNearCoeff( 1.0f );
    light->setShadowFarCoeff( 100.0f );
	scene->attachNode( light.get() );

	Pointer< Camera > camera( new Camera() );
    camera->setRenderPass( new ForwardRenderPass() );
	camera->local().setTranslate( 1.0f, 6.0f, 15.0f );
    camera->local().lookAt( Vector3f( 0.0f, 1.0f, 0.0 ), Vector3f( 0.0f, 1.0f, 0.0f ) );
	scene->attachNode( camera.get() );

	sim->setScene( scene.get() );
	return sim->run();
}

