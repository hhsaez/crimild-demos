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
#include <Crimild_SDL.hpp>
#include <Crimild_Import.hpp>

namespace crimild {

    namespace examples {

        class MusicTrigger : public NodeComponent {
        public:
            MusicTrigger( crimild::Real32 minDistance )
                : _minDistance( minDistance )
            {

            }

            virtual ~MusicTrigger( void )
            {

            }

            virtual void start( void ) override
            {
                auto source = getComponent< AudioSourceComponent >()->getAudioSource();
                source->setLoop( true );
            }

            virtual void update( const Clock &c ) override 
            {
                auto camera = Camera::getMainCamera();

                auto source = getComponent< AudioSourceComponent >()->getAudioSource();

                auto d = Distance::compute( camera->getWorld().getTranslate(), getNode()->getWorld().getTranslate() );
                if ( d <= _minDistance ) {
					source->play();
                }
                else {
					source->pause();
                }
            }

        private:
            crimild::Real32 _minDistance = 1.0f;
        };

		class BackgroundMusic : public NodeComponent {
			CRIMILD_IMPLEMENT_RTTI( crimild::examples::BackgroundMusic )
		public:
			BackgroundMusic( void )
			{

			}

			virtual ~BackgroundMusic( void )
			{

			}

			virtual void start( void ) override
			{
				auto source = getComponent< AudioSourceComponent >()->getAudioSource();
				if ( source != nullptr ) {
					source->setLoop( true );
					source->setVolume( 0.5f );
					source->play();
				}
			}
		};

    }

}

using namespace crimild;
using namespace crimild::sdl;
using namespace crimild::audio;
using namespace crimild::import;

SharedPointer< Node > buildStudio( void )
{
    auto lightBuilder = []( const Vector3f &position ) -> SharedPointer< Node > {
        auto light = crimild::alloc< Light >();
        light->local().setTranslate( position );
        return light;
    };

    SceneImporter importer;
    auto scene = importer.import( FileSystem::getInstance().pathForResource( "assets/models/studio.fbx" ) );
    scene->attachNode( lightBuilder( Vector3f( 4.0, 6.0, -4.0 ) ) );
    scene->attachNode( lightBuilder( Vector3f( -4.0, 6.0, -4.0 ) ) );
    scene->attachNode( lightBuilder( Vector3f( 4.0, 6.0, 4.0 ) ) );
    scene->attachNode( lightBuilder( Vector3f( -4.0, 6.0, 4.0 ) ) );
    return scene;
}

SharedPointer< Node > buildPiano( void )
{
    SceneImporter importer;
    auto scene = importer.import( FileSystem::getInstance().pathForResource( "assets/models/piano.obj" ) );
    scene->local().setTranslate( -3.18606, 0.0, -3.31557 );
    scene->local().rotate().fromEulerAngles( 0.0, 45.0, 0.0 );

    auto trigger = crimild::alloc< Node >();
    trigger->attachComponent< AudioSourceComponent >( AudioManager::getInstance()->createAudioSource( FileSystem::getInstance().pathForResource( "assets/music/piano.wav" ), false ) );
    trigger->attachComponent< crimild::examples::MusicTrigger >( 5.0f );
    trigger->local().setTranslate( 0.0, 3.0f, 0.0 );
    scene->attachNode( trigger );
    
    return scene;    
}

SharedPointer< Node > buildChello( void )
{
    SceneImporter importer;
    auto scene = importer.import( FileSystem::getInstance().pathForResource( "assets/models/cello.fbx" ) );
    scene->local().setTranslate( 4.0, 0.0, -4.0 );
    scene->local().rotate().fromEulerAngles( 0.0, 0.0, 0.0 );

    auto trigger = crimild::alloc< Node >();
    trigger->attachComponent< AudioSourceComponent >( AudioManager::getInstance()->createAudioSource( FileSystem::getInstance().pathForResource( "assets/music/chello.wav" ), false ) );
    trigger->attachComponent< crimild::examples::MusicTrigger >( 3.0f );
    trigger->local().setTranslate( 0.0, 3.0f, 0.0 );
    scene->attachNode( trigger );
    
    return scene;    
}

SharedPointer< Node > buildViolin( void )
{
    SceneImporter importer;
    auto scene = importer.import( FileSystem::getInstance().pathForResource( "assets/models/violin.fbx" ) );
    scene->local().setTranslate( 1.0, 0.0, -1.0 );
    scene->local().rotate().fromEulerAngles( 0.0, 90.0, 0.0 );

    auto trigger = crimild::alloc< Node >();
    trigger->attachComponent< AudioSourceComponent >( AudioManager::getInstance()->createAudioSource( FileSystem::getInstance().pathForResource( "assets/music/violin.wav" ), false ) );
    trigger->attachComponent< crimild::examples::MusicTrigger >( 2.0f );
    trigger->local().setTranslate( 0.0, 0.0f, 0.0 );
    scene->attachNode( trigger );
    
    return scene;    
}

int main( int argc, char **argv )
{
    auto sim = crimild::alloc< SDLSimulation >( "Audio Room", crimild::alloc< Settings >( argc, argv ) );

    auto scene = crimild::alloc< Group >();
    scene->attachNode( buildStudio() );
    scene->attachNode( buildPiano() );
    scene->attachNode( buildChello() );
    scene->attachNode( buildViolin() );

    scene->attachComponent< AudioSourceComponent >( AudioManager::getInstance()->createAudioSource( FileSystem::getInstance().pathForResource( "assets/music/piano.ogg" ), true ) );
	scene->attachComponent< crimild::examples::BackgroundMusic >();

    auto camera = crimild::alloc< Camera >();
    camera->local().setTranslate( Vector3f( 0.0f, 3.0f, 3.0f ) );
    camera->attachComponent< FreeLookCameraComponent >();
    camera->attachComponent< AudioListenerComponent >();
    scene->attachNode( camera );
    
    sim->setScene( scene );

	return sim->run();
}

