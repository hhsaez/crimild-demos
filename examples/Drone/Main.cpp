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

	virtual void onAttach( void ) override
	{
		getNode()->local().setTranslate( 0.0f, 10.0f, -80.0f );
	}

	virtual void update( const Clock &t ) override
	{
		_accumTime += t.getDeltaTime();

		float z = -50.0f + 50.0f * Numericf::sin( _accumTime );
		float y = 5.0f + 1.0f * Numericf::sin( 2.0f * _accumTime );

		getNode()->local().setTranslate( 0.0f, y, z );

		float pitchAngle = -0.15f * Numericf::sin( 0.5f * _accumTime ) * Numericf::cos( 0.5f * _accumTime );
		float rollAngle = 0.025f * Numericf::sin( 4.0f * _accumTime );
		Quaternion4f pitch, roll, yaw;
		pitch.fromAxisAngle( Vector3f( 1.0f, 0.0f, 0.0f ), pitchAngle );
		roll.fromAxisAngle( Vector3f( 0.0f, 0.0f, 1.0f ), rollAngle );
		yaw.fromAxisAngle( Vector3f( 0.0f, 1.0f, 0.0f ), -Numericf::HALF_PI );
		getNode()->local().setRotate( roll * pitch * yaw );
	}

    private:
	float _accumTime = 0.0f;
};

SharedPointer< Node > loadDrone( void )
{
    auto drone = crimild::alloc< Group >( "drone" );
    
	OBJLoader loader( FileSystem::getInstance().pathForResource( "assets/MQ-27b.obj" ) );
	auto droneModel = loader.load();
	if ( droneModel != nullptr ) {
		drone->attachNode( droneModel );
        
		auto droneComponent = crimild::alloc< DroneComponent >();
		drone->attachComponent( droneComponent );
        
//		auto audioClip = crimild::alloc< WavAudioClip >( FileSystem::getInstance().pathForResource( "drone_mono.wav" ) );
//		auto audioComponent = crimild::alloc< AudioComponent >( audioClip );
//		drone->attachComponent( audioComponent );
//		audioComponent->play( true );
	}
    
    return drone;
}

SharedPointer< Node > makeGround( void )
{
	auto primitive = crimild::alloc< QuadPrimitive >( 200.0f, 200.0f );
	auto geometry = crimild::alloc< Geometry >();
	geometry->attachPrimitive( primitive );
	geometry->local().setRotate( Vector3f( 1.0f, 0.0f, 0.0f ), -Numericf::HALF_PI );
    geometry->local().setTranslate( 0.0f, 0.0f, -30.0f );
	
	return geometry;
}

int main( int argc, char **argv )
{
	auto sim = crimild::alloc< GLSimulation >( "Drone", crimild::alloc< Settings >( argc, argv ) );

	auto scene = crimild::alloc< Group >();
    scene->attachNode( loadDrone() );
	scene->attachNode( makeGround() );

	AudioManager::getInstance().setGeneralGain( 80.0f );

	auto light = crimild::alloc< Light >();
	light->local().setTranslate( 10.0f, 25.0f, 20.0f );
    light->local().lookAt( Vector3f( 0.0f, 0.0f, -8.0f ), Vector3f( 0.0f, 1.0f, 0.0 ) );
    light->setCastShadows( true );
    light->setShadowNearCoeff( 1.0f );
    light->setShadowFarCoeff( 100.0f );
	scene->attachNode( light );

	auto camera = crimild::alloc< Camera >( 45.0f, 4.0f / 3.0f, 0.1f, 1024.0f );
	camera->local().setTranslate( 1.0f, 6.0f, 15.0f );
    camera->local().lookAt( Vector3f( 0.0f, 1.0f, 0.0 ), Vector3f( 0.0f, 1.0f, 0.0f ) );
	scene->attachNode( camera );

	sim->setScene( scene );
	return sim->run();
}

