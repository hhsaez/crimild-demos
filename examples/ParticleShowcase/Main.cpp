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

#include "ParticleSystem/ParticleData.hpp"
#include "ParticleSystem/ParticleSystemComponent.hpp"
#include "ParticleSystem/ParticleEmitterComponent.hpp"

#include "ParticleSystem/Generators/BoxPositionParticleGenerator.hpp"
#include "ParticleSystem/Generators/VelocityParticleGenerator.hpp"
#include "ParticleSystem/Generators/SphereVelocityParticleGenerator.hpp"
#include "ParticleSystem/Generators/SpherePositionParticleGenerator.hpp"
#include "ParticleSystem/Generators/AccelerationParticleGenerator.hpp"
#include "ParticleSystem/Generators/ColorParticleGenerator.hpp"
#include "ParticleSystem/Generators/UniformScaleParticleGenerator.hpp"
#include "ParticleSystem/Generators/TimeParticleGenerator.hpp"
#include "ParticleSystem/Generators/NodePositionParticleGenerator.hpp"

#include "ParticleSystem/Updaters/EulerParticleUpdater.hpp"
#include "ParticleSystem/Updaters/TimeParticleUpdater.hpp"
#include "ParticleSystem/Updaters/FloorParticleUpdater.hpp"
#include "ParticleSystem/Updaters/CameraSortParticleUpdater.hpp"
#include "ParticleSystem/Updaters/AttractorParticleUpdater.hpp"
#include "ParticleSystem/Updaters/SetAttribValueParticleUpdater.hpp"

#include "ParticleSystem/Renderers/PointSpriteParticleRendererComponent.hpp"
#include "ParticleSystem/Renderers/OrientedQuadParticleRendererComponent.hpp"
#include "ParticleSystem/Renderers/NodeParticleRendererComponent.hpp"

using namespace crimild;

// TODO: move this to a component
class CameraController : public NodeComponent {
    CRIMILD_IMPLEMENT_RTTI( CameraController )
    
public:
    CameraController( void )
    {

    }
    
    ~CameraController( void )
    {
        
    }
    
    virtual void start( void ) override
    {
        auto sim = Simulation::getInstance();
        
        sim->registerMessageHandler< crimild::messaging::KeyPressed >( []( crimild::messaging::KeyPressed const &msg ) {
            float cameraAxisCoeff = 5.0f;
            
            if ( Input::getInstance()->isKeyDown( CRIMILD_INPUT_KEY_LEFT_SHIFT ) || Input::getInstance()->isKeyDown( CRIMILD_INPUT_KEY_RIGHT_SHIFT ) ) {
                cameraAxisCoeff = 20.0f;
            }
            
            switch ( msg.key ) {
                case 'A':
                case CRIMILD_INPUT_KEY_LEFT:
                    Input::getInstance()->setAxis( Input::AXIS_HORIZONTAL, -cameraAxisCoeff );
                    break;
                    
                case 'D':
                case CRIMILD_INPUT_KEY_RIGHT:
                    Input::getInstance()->setAxis( Input::AXIS_HORIZONTAL, +cameraAxisCoeff );
                    break;
                    
                case 'W':
                case CRIMILD_INPUT_KEY_UP:
                    Input::getInstance()->setAxis( Input::AXIS_VERTICAL, +cameraAxisCoeff );
                    break;
                    
                case 'S':
                case CRIMILD_INPUT_KEY_DOWN:
                    Input::getInstance()->setAxis( Input::AXIS_VERTICAL, -cameraAxisCoeff );
                    break;
            }
        });
        
        sim->registerMessageHandler< crimild::messaging::KeyReleased >( []( crimild::messaging::KeyReleased const &msg ) {
            switch ( msg.key ) {
                case 'K':
                    Simulation::getInstance()->broadcastMessage( crimild::messaging::ToggleDebugInfo { } );
                    break;
                    
                case 'A':
                case 'D':
                case CRIMILD_INPUT_KEY_LEFT:
                case CRIMILD_INPUT_KEY_RIGHT:
                    Input::getInstance()->setAxis( Input::AXIS_HORIZONTAL, 0.0f );
                    break;
                    
                case 'W':
                case CRIMILD_INPUT_KEY_UP:
                case 'S':
                case CRIMILD_INPUT_KEY_DOWN:
                    Input::getInstance()->setAxis( Input::AXIS_VERTICAL, 0.0f );
                    break;
                    
                case CRIMILD_INPUT_KEY_ESCAPE:
                    Simulation::getInstance()->stop();
                    break;
                    
            }
        });
    }
    
    virtual void update( const Clock &c ) override
    {
        Input::getInstance()->setMouseCursorMode( Input::MouseCursorMode::GRAB );
        
        float dSpeed = Input::getInstance()->getAxis( Input::AXIS_VERTICAL );
        float rSpeed = Input::getInstance()->getAxis( Input::AXIS_HORIZONTAL );
        float roll = 0.0f;//Input::getInstance()->getAxis( "CameraAxisRoll" );
        
        auto mouseDelta = Input::getInstance()->getNormalizedMouseDelta();
        
        auto root = getNode();
        
        // apply roll first
        root->local().rotate() *= Quaternion4f::createFromAxisAngle( Vector3f::UNIT_Z, c.getDeltaTime() * roll );
        
        // apply pitch
        auto right = root->getLocal().computeRight();
        root->local().rotate() *= Quaternion4f::createFromAxisAngle( Vector3f::UNIT_X, -mouseDelta[ 1 ] );
        
        // apply yaw
        auto up = root->getLocal().computeWorldUp();
        root->local().rotate() *= Quaternion4f::createFromAxisAngle( up.getNormalized(), -mouseDelta[ 0 ] );
        
        auto direction = root->getLocal().computeDirection();
        
        root->local().translate() += c.getDeltaTime() * ( dSpeed * direction + rSpeed * right );
    }
};

SharedPointer< Node > room( void )
{
	OBJLoader loader( FileSystem::getInstance().pathForResource( "assets/models/room.obj" ) );
	auto model = loader.load();
	return model;
}

SharedPointer< Group > loadModel( std::string filename )
{
	SharedPointer< Group > model;
	auto modelPath = FileSystem::getInstance().pathForResource( filename );
	FileStream is( modelPath, FileStream::OpenMode::READ );
	is.load();
	if ( is.getObjectCount() > 0 ) {
		model = is.getObjectAt< Group >( 0 );
	}
	
	return model;
}

SharedPointer< Node > fire( const Vector3f &position )
{
    const crimild::Size MAX_PARTICLES = 200;
    
    auto ps = crimild::alloc< crimild::Group >();
    
    auto particles = crimild::alloc< ParticleData >( MAX_PARTICLES );
    particles->setAttribs( ParticleAttribType::POSITION, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::VELOCITY, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::ACCELERATION, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::COLOR, crimild::alloc< RGBAColorfParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::START_COLOR, crimild::alloc< RGBAColorfParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::END_COLOR, crimild::alloc< RGBAColorfParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::UNIFORM_SCALE, crimild::alloc< Real32ParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::TIME, crimild::alloc< Real32ParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::LIFE_TIME, crimild::alloc< Real32ParticleAttribArray >() );
    ps->attachComponent< ParticleSystemComponent >( particles );
    
    auto emitter = crimild::alloc< ParticleEmitterComponent >();
    emitter->setEmitRate( 0.75f * MAX_PARTICLES );
    
    auto posGen = crimild::alloc< BoxPositionParticleGenerator >();
    posGen->setOrigin( Vector3f::ZERO );
    posGen->setSize( Vector3f( 2.0f, 0.25f, 2.0f ) );
    emitter->addGenerator( posGen );
    
    auto velGen = crimild::alloc< VelocityParticleGenerator >();
    velGen->setMinVelocity( Vector3f( 0.0f, 1.0f, 0.0f ) );
    velGen->setMaxVelocity( Vector3f( 0.0f, 5.0f, 0.0f ) );
    emitter->addGenerator( velGen );
    
    auto accGen = crimild::alloc< AccelerationParticleGenerator >();
    emitter->addGenerator( accGen );
    
    auto colorGen = crimild::alloc< ColorParticleGenerator >();
    colorGen->setMinStartColor( RGBAColorf( 1.0, 0.0, 0.0, 1.0 ) );
    colorGen->setMaxStartColor( RGBAColorf( 1.0, 1.0, 0.0, 1.0 ) );
    colorGen->setMinEndColor( RGBAColorf( 1.0, 1.0, 1.0, 0.0 ) );
    colorGen->setMaxEndColor( RGBAColorf( 1.0, 1.0, 1.0, 0.0 ) );
    emitter->addGenerator( colorGen );
    
    auto scaleGen = crimild::alloc< UniformScaleParticleGenerator >();
    scaleGen->setMinScale( 50.0f );
    scaleGen->setMaxScale( 200.0f );
    emitter->addGenerator( scaleGen );
    
    auto timeGen = crimild::alloc< TimeParticleGenerator >();
    timeGen->setMinTime( 1.0f );
    timeGen->setMaxTime( 2.0f );
    emitter->addGenerator( timeGen );
    
    ps->attachComponent( emitter );
    
    auto updater = crimild::alloc< ParticleUpdaterComponent >();
    auto eulerUpdater = crimild::alloc< EulerParticleUpdater >();
    updater->addUpdater( eulerUpdater );
    updater->addUpdater( crimild::alloc< TimeParticleUpdater >() );
    ps->attachComponent( updater );
    
    auto renderer = crimild::alloc< PointSpriteParticleRendererComponent >();
    auto texture = crimild::alloc< Texture >( crimild::alloc< ImageTGA >( FileSystem::getInstance().pathForResource( "assets/textures/fire.tga" ) ) );
    renderer->getMaterial()->setColorMap( texture );
    renderer->getMaterial()->setAlphaState( crimild::alloc< AlphaState >( true, AlphaState::SrcBlendFunc::SRC_ALPHA, AlphaState::DstBlendFunc::ONE ) );
    renderer->getMaterial()->setCullFaceState( CullFaceState::DISABLED );
    renderer->getMaterial()->setDepthState( DepthState::DISABLED );
    ps->attachComponent( renderer );
    
    ps->local().setTranslate( position );
    
    return ps;
}

SharedPointer< Node > handsOnFire( const Vector3f &position )
{
    const crimild::Size MAX_PARTICLES = 200;

	auto astroboy = loadModel( "assets/models/astroboy.crimild" );
	astroboy->perform( UpdateWorldState() );
	astroboy->local().setScale( 10.0 / astroboy->getWorldBound()->getRadius() );
	
	std::string jointName = "R_middle_01";
	Group *joint = nullptr;
	astroboy->perform( Apply( [&joint, jointName]( Node *node ) {
		if ( node->getName() == jointName ) {
			joint = static_cast< Group * >( node );
		}
	}));

	assert( joint != nullptr );

    auto ps = crimild::alloc< crimild::Group >();
    
    auto particles = crimild::alloc< ParticleData >( MAX_PARTICLES );
    particles->setAttribs( ParticleAttribType::POSITION, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::VELOCITY, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::ACCELERATION, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::UNIFORM_SCALE, crimild::alloc< Real32ParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::TIME, crimild::alloc< Real32ParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::LIFE_TIME, crimild::alloc< Real32ParticleAttribArray >() );
	particles->setComputeInWorldSpace( true );
    ps->attachComponent< ParticleSystemComponent >( particles );
    
    auto emitter = crimild::alloc< ParticleEmitterComponent >();
    emitter->setEmitRate( 0.25f * MAX_PARTICLES );
    
    auto posGen = crimild::alloc< NodePositionParticleGenerator >();
	posGen->setTargetNode( joint );
    posGen->setSize( Vector3f( 0.25f, 0.25f, 0.25f ) );
    emitter->addGenerator( posGen );
    
    auto velGen = crimild::alloc< VelocityParticleGenerator >();
    velGen->setMinVelocity( Vector3f( 0.0f, 1.0f, 0.0f ) );
    velGen->setMaxVelocity( Vector3f( 0.0f, 2.5f, 0.0f ) );
    emitter->addGenerator( velGen );
    
    auto accGen = crimild::alloc< AccelerationParticleGenerator >();
    emitter->addGenerator( accGen );
    
    auto scaleGen = crimild::alloc< UniformScaleParticleGenerator >();
    scaleGen->setMinScale( 0.25f );
    scaleGen->setMaxScale( 0.75f );
    emitter->addGenerator( scaleGen );
    
    auto timeGen = crimild::alloc< TimeParticleGenerator >();
    timeGen->setMinTime( 0.25f );
    timeGen->setMaxTime( 0.75f );
    emitter->addGenerator( timeGen );
    
    ps->attachComponent( emitter );
    
    auto updater = crimild::alloc< ParticleUpdaterComponent >();
    auto eulerUpdater = crimild::alloc< EulerParticleUpdater >();
    updater->addUpdater( eulerUpdater );
    updater->addUpdater( crimild::alloc< TimeParticleUpdater >() );
    ps->attachComponent( updater );
    
    auto renderer = crimild::alloc< OrientedQuadParticleRendererComponent >();
    auto texture = crimild::alloc< Texture >( crimild::alloc< ImageTGA >( FileSystem::getInstance().pathForResource( "assets/textures/fire.tga" ) ) );
    renderer->getMaterial()->setColorMap( texture );
	renderer->getMaterial()->setDiffuse( RGBAColorf( 1.0f, 0.0f, 0.0f, 1.0f ) );
    renderer->getMaterial()->setAlphaState( crimild::alloc< AlphaState >( true, AlphaState::SrcBlendFunc::SRC_ALPHA, AlphaState::DstBlendFunc::ONE ) );
    renderer->getMaterial()->setCullFaceState( CullFaceState::DISABLED );
    renderer->getMaterial()->setDepthState( DepthState::DISABLED );
    ps->attachComponent( renderer );

	auto scene = crimild::alloc< Group >();
    scene->attachNode( astroboy );
	scene->attachNode( ps );
	
    scene->local().setTranslate( position );
    
    return scene;
}

SharedPointer< Node > explosion( const Vector3f &position )
{
    const crimild::Size MAX_PARTICLES = 500;
    
    auto ps = crimild::alloc< crimild::Group >();
    
    auto particles = crimild::alloc< ParticleData >( MAX_PARTICLES );
    particles->setAttribs( ParticleAttribType::POSITION, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::VELOCITY, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::ACCELERATION, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::COLOR, crimild::alloc< RGBAColorfParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::START_COLOR, crimild::alloc< RGBAColorfParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::END_COLOR, crimild::alloc< RGBAColorfParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::UNIFORM_SCALE, crimild::alloc< Real32ParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::TIME, crimild::alloc< Real32ParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::LIFE_TIME, crimild::alloc< Real32ParticleAttribArray >() );
    ps->attachComponent< ParticleSystemComponent >( particles );
    
    auto emitter = crimild::alloc< ParticleEmitterComponent >();
    emitter->setEmitRate( MAX_PARTICLES );
	emitter->setBurst( true );
    
    auto posGen = crimild::alloc< SpherePositionParticleGenerator >();
    posGen->setOrigin( 5.0f * Vector3f::UNIT_Y );
    posGen->setSize( 0.1f * Vector3f::ONE );
    emitter->addGenerator( posGen );
    
    auto velGen = crimild::alloc< SphereVelocityParticleGenerator >();
    velGen->setMagnitude( 5.0f * Vector3f::ONE );
    emitter->addGenerator( velGen );
    
    auto accGen = crimild::alloc< AccelerationParticleGenerator >();
    emitter->addGenerator( accGen );
    
    auto colorGen = crimild::alloc< ColorParticleGenerator >();
    colorGen->setMinStartColor( RGBAColorf( 0.7, 0.0, 0.7, 1.0 ) );
    colorGen->setMaxStartColor( RGBAColorf( 1.0, 1.0, 1.0, 1.0 ) );
    colorGen->setMinEndColor( RGBAColorf( 0.5, 0.0, 0.6, 0.0 ) );
    colorGen->setMaxEndColor( RGBAColorf( 0.7, 0.5, 0.1, 0.0 ) );
    emitter->addGenerator( colorGen );
    
    auto scaleGen = crimild::alloc< UniformScaleParticleGenerator >();
    scaleGen->setMinScale( 5.0f );
    scaleGen->setMaxScale( 50.0f );
    emitter->addGenerator( scaleGen );
    
    auto timeGen = crimild::alloc< TimeParticleGenerator >();
    timeGen->setMinTime( 3.0f );
    timeGen->setMaxTime( 3.0f );
    emitter->addGenerator( timeGen );
    
    ps->attachComponent( emitter );
    
    auto updater = crimild::alloc< ParticleUpdaterComponent >();
    auto eulerUpdater = crimild::alloc< EulerParticleUpdater >();
    eulerUpdater->setGlobalAcceleration( -10.0f * Vector3f::UNIT_Y );
    updater->addUpdater( eulerUpdater );
    updater->addUpdater( crimild::alloc< TimeParticleUpdater >() );
    updater->addUpdater( crimild::alloc< FloorParticleUpdater >() );
    ps->attachComponent( updater );
    
    auto renderer = crimild::alloc< PointSpriteParticleRendererComponent >();
    ps->attachComponent( renderer );
    
    ps->local().setTranslate( position );
    
    return ps;
}

SharedPointer< Node > flowers( const Vector3f &position )
{
    const crimild::Size MAX_PARTICLES = 500;
    
    auto ps = crimild::alloc< crimild::Group >();
    
    auto particles = crimild::alloc< ParticleData >( MAX_PARTICLES );
    particles->setAttribs( ParticleAttribType::POSITION, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::COLOR, crimild::alloc< RGBAColorfParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::START_COLOR, crimild::alloc< RGBAColorfParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::END_COLOR, crimild::alloc< RGBAColorfParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::UNIFORM_SCALE, crimild::alloc< Real32ParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::TIME, crimild::alloc< Real32ParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::LIFE_TIME, crimild::alloc< Real32ParticleAttribArray >() );
    ps->attachComponent< ParticleSystemComponent >( particles );
    
    auto emitter = crimild::alloc< ParticleEmitterComponent >();
    emitter->setEmitRate( 0.75f * MAX_PARTICLES );
    
    auto posGen = crimild::alloc< BoxPositionParticleGenerator >();
    posGen->setOrigin( Vector3f::ZERO );
    posGen->setSize( Vector3f( 5.0f, 0.0f, 5.0f ) );
    emitter->addGenerator( posGen );
    
    auto colorGen = crimild::alloc< ColorParticleGenerator >();
    colorGen->setMinStartColor( RGBAColorf( 1.0, 1.0, 1.0, 1.0 ) );
    colorGen->setMaxStartColor( RGBAColorf( 1.0, 1.0, 1.0, 1.0 ) );
    colorGen->setMinEndColor( RGBAColorf( 1.0, 1.0, 1.0, 1.0 ) );
    colorGen->setMaxEndColor( RGBAColorf( 1.0, 1.0, 1.0, 1.0 ) );
    emitter->addGenerator( colorGen );
    
    auto scaleGen = crimild::alloc< UniformScaleParticleGenerator >();
    scaleGen->setMinScale( 1.0f );
    scaleGen->setMaxScale( 2.0f );
    emitter->addGenerator( scaleGen );
    
    ps->attachComponent( emitter );

	auto updater = crimild::alloc< ParticleUpdaterComponent >();
	updater->addUpdater( crimild::alloc< CameraSortParticleUpdater >() );
	ps->attachComponent( updater );
    
    auto renderer = crimild::alloc< OrientedQuadParticleRendererComponent >();
    auto texture = crimild::alloc< Texture >( crimild::alloc< ImageTGA >( FileSystem::getInstance().pathForResource( "assets/textures/jasmine.tga" ) ) );
    renderer->getMaterial()->setColorMap( texture );
    renderer->getMaterial()->setAlphaState( AlphaState::ENABLED );
    renderer->getMaterial()->setCullFaceState( CullFaceState::DISABLED );
    ps->attachComponent( renderer );
    
    ps->local().setTranslate( position );
    
    return ps;
}

SharedPointer< Node > smoke( const Vector3f &position, bool computeInWorldSpace = false )
{
    const crimild::Size MAX_PARTICLES = 200;
    
    auto ps = crimild::alloc< crimild::Group >();
    
    auto particles = crimild::alloc< ParticleData >( MAX_PARTICLES );
    particles->setAttribs( ParticleAttribType::POSITION, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::VELOCITY, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::ACCELERATION, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::COLOR, crimild::alloc< RGBAColorfParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::START_COLOR, crimild::alloc< RGBAColorfParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::END_COLOR, crimild::alloc< RGBAColorfParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::UNIFORM_SCALE, crimild::alloc< Real32ParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::TIME, crimild::alloc< Real32ParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::LIFE_TIME, crimild::alloc< Real32ParticleAttribArray >() );
	particles->setComputeInWorldSpace( computeInWorldSpace );
    ps->attachComponent< ParticleSystemComponent >( particles );
    
    auto emitter = crimild::alloc< ParticleEmitterComponent >();
    emitter->setEmitRate( 0.25f * MAX_PARTICLES );
    
    auto posGen = crimild::alloc< BoxPositionParticleGenerator >();
    posGen->setOrigin( Vector3f::ZERO );
    posGen->setSize( 0.05f * Vector3f::ONE );
    emitter->addGenerator( posGen );
    
    auto velGen = crimild::alloc< VelocityParticleGenerator >();
    velGen->setMinVelocity( Vector3f( 0.0f, 1.0f, 0.0f ) );
    velGen->setMaxVelocity( Vector3f( 0.0f, 2.0f, 0.0f ) );
    emitter->addGenerator( velGen );
    
    auto accGen = crimild::alloc< AccelerationParticleGenerator >();
    emitter->addGenerator( accGen );
    
    auto colorGen = crimild::alloc< ColorParticleGenerator >();
    colorGen->setMinStartColor( RGBAColorf( 1.0, 1.0, 1.0, 1.0 ) );
    colorGen->setMaxStartColor( RGBAColorf( 1.0, 1.0, 1.0, 1.0 ) );
    colorGen->setMinEndColor( RGBAColorf( 1.0, 1.0, 1.0, 0.0 ) );
    colorGen->setMaxEndColor( RGBAColorf( 1.0, 1.0, 1.0, 0.0 ) );
    emitter->addGenerator( colorGen );
    
    auto scaleGen = crimild::alloc< UniformScaleParticleGenerator >();
    scaleGen->setMinScale( 50.0f );
    scaleGen->setMaxScale( 200.0f );
    emitter->addGenerator( scaleGen );
    
    auto timeGen = crimild::alloc< TimeParticleGenerator >();
    timeGen->setMinTime( 1.0f );
    timeGen->setMaxTime( 2.0f );
    emitter->addGenerator( timeGen );
    
    ps->attachComponent( emitter );
    
    auto updater = crimild::alloc< ParticleUpdaterComponent >();
    auto eulerUpdater = crimild::alloc< EulerParticleUpdater >();
    updater->addUpdater( eulerUpdater );
    updater->addUpdater( crimild::alloc< TimeParticleUpdater >() );
    ps->attachComponent( updater );
    
    auto renderer = crimild::alloc< PointSpriteParticleRendererComponent >();
    auto texture = crimild::alloc< Texture >( crimild::alloc< ImageTGA >( FileSystem::getInstance().pathForResource( "assets/textures/smoke.tga" ) ) );
    renderer->getMaterial()->setColorMap( texture );
    renderer->getMaterial()->setAlphaState( crimild::alloc< AlphaState >( true, AlphaState::SrcBlendFunc::SRC_ALPHA, AlphaState::DstBlendFunc::ONE ) );
    renderer->getMaterial()->setCullFaceState( CullFaceState::DISABLED );
    renderer->getMaterial()->setDepthState( DepthState::DISABLED );
    ps->attachComponent( renderer );
    
    ps->attachComponent< OrbitComponent >( 0.0f, 0.0f, 5.0f, 3.0f, 2.0f, 1.0f );

	auto group = crimild::alloc< Group >();
	group->attachNode( ps );
	group->local().setTranslate( position );
    
    return group;
}

SharedPointer< Node > fountain( const Vector3f &position )
{
    const crimild::Size MAX_PARTICLES = 500;
    
    auto ps = crimild::alloc< crimild::Group >();
    
    auto particles = crimild::alloc< ParticleData >( MAX_PARTICLES );
    particles->setAttribs( ParticleAttribType::POSITION, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::VELOCITY, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::ACCELERATION, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::COLOR, crimild::alloc< RGBAColorfParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::START_COLOR, crimild::alloc< RGBAColorfParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::END_COLOR, crimild::alloc< RGBAColorfParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::UNIFORM_SCALE, crimild::alloc< Real32ParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::TIME, crimild::alloc< Real32ParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::LIFE_TIME, crimild::alloc< Real32ParticleAttribArray >() );
    ps->attachComponent< ParticleSystemComponent >( particles );
    
    auto emitter = crimild::alloc< ParticleEmitterComponent >();
    emitter->setEmitRate( 0.75f * MAX_PARTICLES );
    
    auto posGen = crimild::alloc< BoxPositionParticleGenerator >();
    posGen->setOrigin( Vector3f::ZERO );
    posGen->setSize( Vector3f( 5.0f, 0.1f, 5.0f ) );
    emitter->addGenerator( posGen );
    
    auto velGen = crimild::alloc< VelocityParticleGenerator >();
    velGen->setMinVelocity( Vector3f( 0.0f, 1.0f, 0.0f ) );
    velGen->setMaxVelocity( Vector3f( 0.0f, 10.0f, 0.0f ) );
    emitter->addGenerator( velGen );
    
    auto accGen = crimild::alloc< AccelerationParticleGenerator >();
    accGen->setMinAcceleration( Vector3f::ZERO );
    accGen->setMaxAcceleration( Vector3f::ZERO );
    emitter->addGenerator( accGen );
    
    auto colorGen = crimild::alloc< ColorParticleGenerator >();
    colorGen->setMinStartColor( RGBAColorf( 0.7, 0.0, 0.7, 1.0 ) );
    colorGen->setMaxStartColor( RGBAColorf( 1.0, 1.0, 1.0, 1.0 ) );
    colorGen->setMinEndColor( RGBAColorf( 0.5, 0.0, 0.6, 0.0 ) );
    colorGen->setMaxEndColor( RGBAColorf( 0.7, 0.5, 0.1, 0.0 ) );
    emitter->addGenerator( colorGen );
    
    auto scaleGen = crimild::alloc< UniformScaleParticleGenerator >();
    scaleGen->setMinScale( 5.0f );
    scaleGen->setMaxScale( 50.0f );
    emitter->addGenerator( scaleGen );
    
    auto timeGen = crimild::alloc< TimeParticleGenerator >();
    timeGen->setMinTime( 1.0f );
    timeGen->setMaxTime( 5.0f );
    emitter->addGenerator( timeGen );
    
    ps->attachComponent( emitter );
    
    auto updater = crimild::alloc< ParticleUpdaterComponent >();
    auto eulerUpdater = crimild::alloc< EulerParticleUpdater >();
    eulerUpdater->setGlobalAcceleration( -10.0f * Vector3f::UNIT_Y );
    updater->addUpdater( eulerUpdater );
    updater->addUpdater( crimild::alloc< TimeParticleUpdater >() );
    updater->addUpdater( crimild::alloc< FloorParticleUpdater >() );
    ps->attachComponent( updater );
    
    auto renderer = crimild::alloc< PointSpriteParticleRendererComponent >();
    ps->attachComponent( renderer );
    
    ps->local().setTranslate( position );
    
    return ps;
}

SharedPointer< Node > sprinklers( const Vector3f &position )
{
    const crimild::Size MAX_PARTICLES = 500;
    
    auto ps = crimild::alloc< crimild::Group >();
    
    auto particles = crimild::alloc< ParticleData >( MAX_PARTICLES );
    particles->setAttribs( ParticleAttribType::POSITION, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::VELOCITY, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::ACCELERATION, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::COLOR, crimild::alloc< RGBAColorfParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::START_COLOR, crimild::alloc< RGBAColorfParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::END_COLOR, crimild::alloc< RGBAColorfParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::UNIFORM_SCALE, crimild::alloc< Real32ParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::TIME, crimild::alloc< Real32ParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::LIFE_TIME, crimild::alloc< Real32ParticleAttribArray >() );
	particles->setComputeInWorldSpace( true );
    ps->attachComponent< ParticleSystemComponent >( particles );
    
    auto emitter = crimild::alloc< ParticleEmitterComponent >();
    emitter->setEmitRate( 0.25f * MAX_PARTICLES );
    
    auto posGen = crimild::alloc< BoxPositionParticleGenerator >();
    posGen->setOrigin( Vector3f::ZERO );
    posGen->setSize( 0.5f * Vector3f::ONE );
    emitter->addGenerator( posGen );
    
    auto velGen = crimild::alloc< VelocityParticleGenerator >();
    velGen->setMinVelocity( Vector3f( -3.0f, 5.0f, -3.0f ) );
    velGen->setMaxVelocity( Vector3f( 3.0f, 8.0f, 3.0f ) );
    emitter->addGenerator( velGen );
    
    auto accGen = crimild::alloc< AccelerationParticleGenerator >();
    accGen->setMinAcceleration( Vector3f::ZERO );
    accGen->setMaxAcceleration( Vector3f::ZERO );
    emitter->addGenerator( accGen );
    
    auto colorGen = crimild::alloc< ColorParticleGenerator >();
    colorGen->setMinStartColor( RGBAColorf( 0.0, 0.0, 0.7, 1.0 ) );
    colorGen->setMaxStartColor( RGBAColorf( 1.0, 1.0, 1.0, 1.0 ) );
    colorGen->setMinEndColor( RGBAColorf( 0.0, 0.0, 0.25, 0.0 ) );
    colorGen->setMaxEndColor( RGBAColorf( 0.0, 0.0, 0.7, 0.0 ) );
    emitter->addGenerator( colorGen );
    
    auto scaleGen = crimild::alloc< UniformScaleParticleGenerator >();
    scaleGen->setMinScale( 10.0f );
    scaleGen->setMaxScale( 20.0f );
    emitter->addGenerator( scaleGen );
    
    auto timeGen = crimild::alloc< TimeParticleGenerator >();
    timeGen->setMinTime( 1.0f );
    timeGen->setMaxTime( 1.5f );
    emitter->addGenerator( timeGen );
    
    ps->attachComponent( emitter );
    
    auto updater = crimild::alloc< ParticleUpdaterComponent >();
    auto eulerUpdater = crimild::alloc< EulerParticleUpdater >();
    eulerUpdater->setGlobalAcceleration( -10.0f * Vector3f::UNIT_Y );
    updater->addUpdater( eulerUpdater );
    updater->addUpdater( crimild::alloc< TimeParticleUpdater >() );
    updater->addUpdater( crimild::alloc< FloorParticleUpdater >() );
    ps->attachComponent( updater );
    
    auto renderer = crimild::alloc< PointSpriteParticleRendererComponent >();
    ps->attachComponent( renderer );
    
    ps->local().setTranslate( position );

	ps->attachComponent< RotationComponent >( Vector3f::UNIT_Y, 0.1f );
    
    return ps;
}

SharedPointer< Node > sparkles( const Vector3f &position )
{
    const crimild::Size MAX_PARTICLES = 500;
    
    auto ps = crimild::alloc< crimild::Group >();
    
    auto particles = crimild::alloc< ParticleData >( MAX_PARTICLES );
    particles->setAttribs( ParticleAttribType::POSITION, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::VELOCITY, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::ACCELERATION, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::COLOR, crimild::alloc< RGBAColorfParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::START_COLOR, crimild::alloc< RGBAColorfParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::END_COLOR, crimild::alloc< RGBAColorfParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::UNIFORM_SCALE, crimild::alloc< Real32ParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::TIME, crimild::alloc< Real32ParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::LIFE_TIME, crimild::alloc< Real32ParticleAttribArray >() );
	particles->setComputeInWorldSpace( true );
    ps->attachComponent< ParticleSystemComponent >( particles );
    
    auto emitter = crimild::alloc< ParticleEmitterComponent >();
    emitter->setEmitRate( 0.25f * MAX_PARTICLES );
    
    auto posGen = crimild::alloc< BoxPositionParticleGenerator >();
    posGen->setOrigin( Vector3f::ZERO );
    posGen->setSize( 0.5f * Vector3f::ONE );
    emitter->addGenerator( posGen );
    
    auto velGen = crimild::alloc< VelocityParticleGenerator >();
    velGen->setMinVelocity( Vector3f( 0.0, 5.0f, -8.0f ) );
    velGen->setMaxVelocity( Vector3f( 0.0, 8.0f, 8.0f ) );
    emitter->addGenerator( velGen );
    
    auto accGen = crimild::alloc< AccelerationParticleGenerator >();
    accGen->setMinAcceleration( Vector3f::ZERO );
    accGen->setMaxAcceleration( Vector3f::ZERO );
    emitter->addGenerator( accGen );
    
    auto colorGen = crimild::alloc< ColorParticleGenerator >();
    colorGen->setMinStartColor( RGBAColorf( 1.0, 0.0, 0.0, 1.0 ) );
    colorGen->setMaxStartColor( RGBAColorf( 1.0, 1.0, 1.0, 1.0 ) );
    colorGen->setMinEndColor( RGBAColorf( 0.75, 0.0, 0.0, 0.0 ) );
    colorGen->setMaxEndColor( RGBAColorf( 0.9, 0.5, 0.0, 0.0 ) );
    emitter->addGenerator( colorGen );
    
    auto scaleGen = crimild::alloc< UniformScaleParticleGenerator >();
    scaleGen->setMinScale( 20.0f );
    scaleGen->setMaxScale( 50.0f );
    emitter->addGenerator( scaleGen );
    
    auto timeGen = crimild::alloc< TimeParticleGenerator >();
    timeGen->setMinTime( 1.0f );
    timeGen->setMaxTime( 3.0f );
    emitter->addGenerator( timeGen );
    
    ps->attachComponent( emitter );
    
    auto updater = crimild::alloc< ParticleUpdaterComponent >();
    auto eulerUpdater = crimild::alloc< EulerParticleUpdater >();
    eulerUpdater->setGlobalAcceleration( -10.0f * Vector3f::UNIT_Y );
    updater->addUpdater( eulerUpdater );
    updater->addUpdater( crimild::alloc< TimeParticleUpdater >() );
    ps->attachComponent( updater );
    
    auto renderer = crimild::alloc< PointSpriteParticleRendererComponent >();
    ps->attachComponent( renderer );
    
    ps->local().setTranslate( position );

    return ps;
}

SharedPointer< Node > attractors( const Vector3f &position )
{
    const crimild::Size MAX_PARTICLES = 500;
    
    auto ps = crimild::alloc< crimild::Group >();
    
    auto particles = crimild::alloc< ParticleData >( MAX_PARTICLES );
    particles->setAttribs( ParticleAttribType::POSITION, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::VELOCITY, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::ACCELERATION, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::COLOR, crimild::alloc< RGBAColorfParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::START_COLOR, crimild::alloc< RGBAColorfParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::END_COLOR, crimild::alloc< RGBAColorfParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::UNIFORM_SCALE, crimild::alloc< Real32ParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::TIME, crimild::alloc< Real32ParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::LIFE_TIME, crimild::alloc< Real32ParticleAttribArray >() );
	particles->setComputeInWorldSpace( true );
    ps->attachComponent< ParticleSystemComponent >( particles );
    
    auto emitter = crimild::alloc< ParticleEmitterComponent >();
    emitter->setEmitRate( MAX_PARTICLES );
	emitter->setBurst( true );
    
    auto posGen = crimild::alloc< SpherePositionParticleGenerator >();
    posGen->setOrigin( Vector3f::ZERO );
    posGen->setSize( 0.1f * Vector3f::ONE );
    emitter->addGenerator( posGen );
    
    auto velGen = crimild::alloc< SphereVelocityParticleGenerator >();
	velGen->setMagnitude( 0.0f * Vector3f::ONE );
    emitter->addGenerator( velGen );
    
    auto accGen = crimild::alloc< AccelerationParticleGenerator >();
    accGen->setMinAcceleration( Vector3f::ZERO );
    accGen->setMaxAcceleration( Vector3f::ZERO );
    emitter->addGenerator( accGen );
    
    auto colorGen = crimild::alloc< ColorParticleGenerator >();
    colorGen->setMinStartColor( RGBAColorf( 0.0, 1.0, 0.0, 1.0 ) );
    colorGen->setMaxStartColor( RGBAColorf( 1.0, 1.0, 1.0, 1.0 ) );
    colorGen->setMinEndColor( RGBAColorf( 0.0, 0.75, 0.0, 0.0 ) );
    colorGen->setMaxEndColor( RGBAColorf( 0.5, 0.95, 0.0, 0.0 ) );
    emitter->addGenerator( colorGen );
    
    auto scaleGen = crimild::alloc< UniformScaleParticleGenerator >();
    scaleGen->setMinScale( 10.0f );
    scaleGen->setMaxScale( 20.0f );
    emitter->addGenerator( scaleGen );
    
    auto timeGen = crimild::alloc< TimeParticleGenerator >();
    timeGen->setMinTime( 30.0f );
    timeGen->setMaxTime( 30.0f );
    emitter->addGenerator( timeGen );
    
    ps->attachComponent( emitter );
    
    auto updater = crimild::alloc< ParticleUpdaterComponent >();
	auto resetAccelerations = crimild::alloc< SetAttribValueParticleUpdater< Vector3f >>();
	resetAccelerations->setAttribType( ParticleAttribType::ACCELERATION );
	resetAccelerations->setValue( Vector3f::ZERO );
	updater->addUpdater( resetAccelerations );
	auto attractor = crimild::alloc< AttractorParticleUpdater >();
	attractor->setAttractor( Sphere3f( position + Vector3f( 0.0f, 3.0f, 0.0f ), 5.0f ) );
	attractor->setStrength( 50.0f );
	updater->addUpdater( attractor );
	auto repulsor = crimild::alloc< AttractorParticleUpdater >();
	repulsor->setAttractor( Sphere3f( position, 2.0f ) );
	repulsor->setStrength( -25.0f );
	updater->addUpdater( repulsor );
    auto eulerUpdater = crimild::alloc< EulerParticleUpdater >();
    updater->addUpdater( eulerUpdater );
    updater->addUpdater( crimild::alloc< TimeParticleUpdater >() );
    ps->attachComponent( updater );
    
    auto renderer = crimild::alloc< PointSpriteParticleRendererComponent >();
    ps->attachComponent( renderer );
    
    ps->local().setTranslate( position );

    return ps;
}

SharedPointer< Node > walkers( const Vector3f &position )
{
    const crimild::Size MAX_PARTICLES = 10;
    
    auto ps = crimild::alloc< crimild::Group >();

	auto astroboy = loadModel( "assets/models/astroboy.crimild" );

	astroboy->perform( UpdateWorldState() );
	const auto modelScale = 3.0f / astroboy->getWorldBound()->getRadius();

	for ( int i = 0; i < MAX_PARTICLES; i++ ) {
		ShallowCopy copier;
		astroboy->perform( copier );
		auto copy = copier.getResult< Node >();
		copy->local().setScale( modelScale );
		ps->attachNode( copy );
	}
    
    auto particles = crimild::alloc< ParticleData >( MAX_PARTICLES );
    particles->setAttribs( ParticleAttribType::POSITION, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::VELOCITY, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::ACCELERATION, crimild::alloc< Vector3fParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::TIME, crimild::alloc< Real32ParticleAttribArray >() );
    particles->setAttribs( ParticleAttribType::LIFE_TIME, crimild::alloc< Real32ParticleAttribArray >() );
    ps->attachComponent< ParticleSystemComponent >( particles );
    
    auto emitter = crimild::alloc< ParticleEmitterComponent >();
    emitter->setEmitRate( 0.75f * MAX_PARTICLES );
    
    auto posGen = crimild::alloc< BoxPositionParticleGenerator >();
    posGen->setOrigin( Vector3f::ZERO );
    posGen->setSize( Vector3f( 5.0f, 0.1f, 5.0f ) );
    emitter->addGenerator( posGen );
    
    auto velGen = crimild::alloc< VelocityParticleGenerator >();
    velGen->setMinVelocity( Vector3f( -2.0f, 0.0f, -2.0f ) );
    velGen->setMaxVelocity( Vector3f( 2.0f, 0.0f, 2.0f ) );
    emitter->addGenerator( velGen );
    
    auto accGen = crimild::alloc< AccelerationParticleGenerator >();
    accGen->setMinAcceleration( Vector3f::ZERO );
    accGen->setMaxAcceleration( Vector3f::ZERO );
    emitter->addGenerator( accGen );
    
    auto timeGen = crimild::alloc< TimeParticleGenerator >();
    timeGen->setMinTime( 10.0f );
    timeGen->setMaxTime( 15.0f );
    emitter->addGenerator( timeGen );
    
    ps->attachComponent( emitter );
    
    auto updater = crimild::alloc< ParticleUpdaterComponent >();
    auto eulerUpdater = crimild::alloc< EulerParticleUpdater >();
    updater->addUpdater( eulerUpdater );
    updater->addUpdater( crimild::alloc< TimeParticleUpdater >() );
    ps->attachComponent( updater );
    
    auto renderer = crimild::alloc< NodeParticleRendererComponent >();
    ps->attachComponent( renderer );
    
    ps->local().setTranslate( position );
    
    return ps;
}

int main( int argc, char **argv )
{
    auto sim = crimild::alloc< GLSimulation >( "Particle Showcase", crimild::alloc< Settings >( argc, argv ) );

    auto scene = crimild::alloc< Group >();

	scene->attachNode( room() );

    scene->attachNode( fire( Vector3f( -10.0f, 0.5f, -10.0f ) ) );
    scene->attachNode( flowers( Vector3f( -10.0f, 1.0f, -20.0f ) ) );
    scene->attachNode( sprinklers( Vector3f( -10.0f, 2.0f, -20.0f ) ) );
	scene->attachNode( explosion( Vector3f( -10.0f, 0.0f, -60.0f ) ) );
	scene->attachNode( attractors( Vector3f( -10.0f, 5.0f, -40.0f ) ) );

    scene->attachNode( fountain( Vector3f( 10.0f, 0.5f, -10.0f ) ) );
    scene->attachNode( smoke( Vector3f( 5.0f, 5.0f, -50.0f ), false ) );
    scene->attachNode( smoke( Vector3f( 15.0f, 5.0f, -50.0f ), true ) );
	scene->attachNode( sparkles( Vector3f( 15.0f, 10.0f, -30.0f ) ) );

	scene->attachNode( handsOnFire( Vector3f( 0.0f, 0.0f, -60.0f ) ) );
	scene->attachNode( walkers( Vector3f( 0.0f, 0.0f, -30.0f ) ) );

    auto camera = crimild::alloc< Camera >();
    camera->local().setTranslate( Vector3f( 0.0f, 10.0f, 10.0f ) );
    camera->attachComponent< CameraController >();
	camera->setCullingEnabled( false );
    scene->attachNode( camera );
    
    sim->setScene( scene );
	
	return sim->run();
}

