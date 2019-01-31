/*
 * Copyright (c) 2002-present, H. Hernán Saez
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
#include <Crimild_OpenGL.hpp>

#include "Rendering/RenderGraph/RenderGraph.hpp"
#include "Rendering/RenderGraph/RenderGraphPass.hpp"
#include "Rendering/RenderGraph/RenderGraphAttachment.hpp"
#include "Rendering/RenderGraph/Passes/ForwardLightingPass.hpp"
#include "Rendering/RenderGraph/Passes/ScreenPass.hpp"
#include "Rendering/RenderGraph/Passes/DepthPass.hpp"
#include "Rendering/RenderGraph/Passes/BlendPass.hpp"
#include "Rendering/RenderGraph/Passes/DepthToRGBPass.hpp"
#include "Rendering/RenderGraph/Passes/FrameDebugPass.hpp"
#include "Rendering/RenderGraph/Passes/OpaquePass.hpp"
#include "Rendering/RenderGraph/Passes/LightAccumulationPass.hpp"
#include "Rendering/RenderGraph/Passes/TextureColorPass.hpp"
#include "Rendering/RenderGraph/Passes/DeferredLightingPass.hpp"

using namespace crimild;
using namespace crimild::rendergraph;
using namespace crimild::rendergraph::passes;

class FloatingComponent : public NodeComponent {
public:
    FloatingComponent( void )
    {

    }

    virtual ~FloatingComponent( void )
    {

    }

    virtual void start( void ) override
    {
        _position = getNode()->getLocal().getTranslate();
        _theta = Random::generate< crimild::Real32 >( Numericf::PI );
        _speed = Random::generate< crimild::Real32 >( 0.5f, 2.0f );
    }

    virtual void update( const Clock &c ) override
    {
        _theta += _speed * c.getDeltaTime();
        getNode()->local().setTranslate( _position + Numericf::sin( _theta ) * Vector3f::UNIT_Y );
    }

private:
    Vector3f _position;
    crimild::Real32 _theta;
    crimild::Real32 _speed;
};

SharedPointer< RenderGraph > createRenderGraph( crimild::Bool debugEnabled = false )
{
    auto graph = crimild::alloc< RenderGraph >();

    auto scenePass = graph->createPass< ForwardLightingPass >();
    auto shadowPass = graph->createPass< ShadowPass >();

    scenePass->setShadowInput( shadowPass->getShadowOutput() );

    graph->setOutput( scenePass->getColorOutput() );

    if ( debugEnabled ) {
        auto depthPass = graph->createPass< DepthPass >();
        scenePass->setDepthInput( depthPass->getDepthOutput() );

        auto linearizeDepthPass = graph->createPass< LinearizeDepthPass >();
        linearizeDepthPass->setInput( depthPass->getDepthOutput() );

        auto shadowMap = graph->createPass< TextureColorPass >( TextureColorPass::Mode::RED );
        shadowMap->setInput( shadowPass->getShadowOutput() );

        auto debugPass = graph->createPass< FrameDebugPass >();
        debugPass->addInput( shadowMap->getOutput() );
        debugPass->addInput( scenePass->getColorOutput() );
        debugPass->addInput( linearizeDepthPass->getOutput() );
        graph->setOutput( debugPass->getOutput() );
    }

    return graph;
}

SharedPointer< Node > buildAmbientLight( const RGBAColorf &color )
{
	auto light = crimild::alloc< Light >( Light::Type::AMBIENT );
	light->setAmbient( color );
	return light;
}

SharedPointer< Node > buildSpotlight( const Vector3f &position )
{
	auto light = crimild::alloc< Light >( Light::Type::SPOT );
	light->setInnerCutoff( Numericf::DEG_TO_RAD * 25.0f );
	light->setOuterCutoff( Numericf::DEG_TO_RAD * 50.0f );

	light->setColor( RGBAColorf( 1.0f, 0.0f, 0.0f, 1.0f ) );

    light->setCastShadows( true );

    light->local().setTranslate( position );
    light->local().lookAt( Vector3f::ZERO );

    return light;
}

SharedPointer< Node > buildCube( const Vector3f &position, const Vector3f size = Vector3f::ONE, crimild::Bool animated = true )
{
	auto geometry = crimild::alloc< Geometry >();
	geometry->attachPrimitive( crimild::alloc< BoxPrimitive >( size.x(), size.y(), size.z(), VertexFormat::VF_P3_N3 ) );

	auto material = crimild::alloc< Material >();
	material->setAmbient( RGBAColorf::ONE );
	material->setDiffuse( RGBAColorf::ONE );
	geometry->getComponent< MaterialComponent >()->attachMaterial( material );

	geometry->local().setTranslate( position );

    if ( animated ) {
        geometry->attachComponent< FloatingComponent >();
    }

	return geometry;
}

SharedPointer< Node > buildWall( const Vector3f &position, const Vector3f size = Vector3f::ONE )
{
    auto geometry = crimild::alloc< Geometry >();
    geometry->attachPrimitive( crimild::alloc< BoxPrimitive >( size.x(), size.y(), size.z(), VertexFormat::VF_P3_N3 ) );

    auto material = crimild::alloc< Material >();
    material->setAmbient( RGBAColorf::ONE );
    material->setDiffuse( RGBAColorf::ZERO );
    material->setSpecular( RGBAColorf::ZERO );
    geometry->getComponent< MaterialComponent >()->attachMaterial( material );

    geometry->local().setTranslate( position );

    return geometry;
}

int main( int argc, char **argv )
{
	crimild::init();
	
    auto settings = crimild::alloc< Settings >( argc, argv );
    settings->set( "video.show_frame_time", true );
    CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< sdl::SDLSimulation >( "Spotlight", settings );

    auto scene = crimild::alloc< Group >();

    scene->attachNode( buildCube( Vector3f( 0.0f, -2.0f, -100.0f ), Vector3f( 50.0f, 0.1f, 500.0f ), false ) );
    scene->attachNode( buildWall( Vector3f( -25.0f, 50.0f, -100.0f ), Vector3f( 0.1f, 200.0f, 500.0f ) ) );
    scene->attachNode( buildWall( Vector3f( 0.0f, 0.0f, -500.0f ), Vector3f( 500.1f, 500.0f, 0.1f ) ) );

    scene->attachNode( buildCube( Vector3f( -2.0f, 0.75f, -3.0f ), 1.25f * Vector3f::ONE ) );
    scene->attachNode( buildCube( Vector3f( -2.5f, 0.5f, 0.0f ), 1.5f * Vector3f::ONE ) );
	scene->attachNode( buildCube( Vector3f( -1.0f, 0.25f, 3.0f ), Vector3f::ONE ) );

    scene->attachNode( buildCube( Vector3f( 0.5f, 0.35f, -3.0f ) ) );
    scene->attachNode( buildCube( Vector3f( 0.0f, 0.0f, 0.0f ) ) );
    scene->attachNode( buildCube( Vector3f( 0.75f, 0.5f, 3.0f ) ) );

    scene->attachNode( buildCube( Vector3f( 4.0f, 0.5f, -3.0f ) ) );
    scene->attachNode( buildCube( Vector3f( 3.3f, 0.25f, 0.0f ) ) );
    scene->attachNode( buildCube( Vector3f( 3.0f, 0.75f, 3.0f ) ) );

    scene->attachNode( buildAmbientLight( RGBAColorf( 0.15f, 0.0f, 0.1f, 1.0f ) ) );
    scene->attachNode( buildSpotlight( Vector3f( -5.0f, 3.0f, 5.0f ) ) );

	auto camera = crimild::alloc< Camera >();
	camera->local().setTranslate( 20.0f, 5.0f, 10.0f );
    camera->local().lookAt( Vector3f( 0.0f, 0.0f, -10.0f ) );
    auto renderGraph = createRenderGraph();
    camera->setRenderPass( crimild::alloc< RenderGraphRenderPass >( renderGraph ) );
	scene->attachNode( camera );
    
    sim->setScene( scene );
	
	return sim->run();
}

