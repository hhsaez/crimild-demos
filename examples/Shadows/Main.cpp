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

#include <fstream>
#include <string>
#include <vector>

using namespace crimild;
using namespace crimild::rendergraph;
using namespace crimild::rendergraph::passes;
using namespace crimild::sdl;

const RGBAColorf COLOR_BACKGROUND = RGBAColorf( 247.0f / 255.0f, 92.0f / 255.0f, 3.0f / 255.0f, 1.0f );
const RGBAColorf COLOR_FLOOR = RGBAColorf( 253.0f / 255.0f, 246.0f / 255.0f, 236.0f / 255.0f, 1.0f );
const RGBAColorf COLOR_WALL = RGBAColorf( 254.0f / 255.0f, 220.0f / 255.0f, 172.0f / 255.0f, 1.0f );
const RGBAColorf COLOR_PILLAR = RGBAColorf( 244.0f / 255.0f, 113.0f / 255.0f, 112.0f / 255.0f, 1.0f );
const RGBAColorf COLOR_TEAPOT = RGBAColorf( 187.0f / 255.0f, 55.0f / 255.0f, 101.0f / 255.0f, 1.0f );

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

SharedPointer< Node > buildBackground( void )
{
    auto background = crimild::alloc< Group >();

    auto geometry = crimild::alloc< Geometry >();
    geometry->attachPrimitive( crimild::alloc< QuadPrimitive >( 200.0f, 200.0f ) );
    geometry->local().setTranslate( -50.0f * Vector3f::UNIT_Z );

    auto material = crimild::alloc< Material >();
    material->setDiffuse( COLOR_BACKGROUND );
    material->setProgram( crimild::alloc< UnlitShaderProgram >() );
    material->setCastShadows( false );
    geometry->getComponent< MaterialComponent >()->attachMaterial( material );

    background->attachNode( geometry );

    auto ambientLight = crimild::alloc< Light >( Light::Type::AMBIENT );
    ambientLight->setAmbient( 0.1f * COLOR_BACKGROUND );
    background->attachNode( ambientLight );

    return background;
}

SharedPointer< Node > buildFloor( void )
{
    auto geometry = crimild::alloc< Geometry >();
    geometry->attachPrimitive( crimild::alloc< BoxPrimitive >( 50.0f, 0.1f, 50.0f, VertexFormat::VF_P3_N3 ) );
    geometry->local().setTranslate( -0.05f * Vector3f::UNIT_Y );

    auto material = crimild::alloc< Material >();
    material->setAmbient( RGBAColorf::ONE );
    material->setDiffuse( COLOR_FLOOR );
    material->setSpecular( RGBAColorf::ZERO );
    geometry->getComponent< MaterialComponent >()->attachMaterial( material );

    return geometry;
}

SharedPointer< Node > buildWall( void )
{
    auto scene = crimild::alloc< Group >();

    auto material = crimild::alloc< Material >();
    material->setAmbient( RGBAColorf::ONE );
    material->setDiffuse( COLOR_WALL );

    auto floor = crimild::alloc< Geometry >();
    floor->attachPrimitive( crimild::alloc< BoxPrimitive >( 20.0f, 0.25f, 20.0f, VertexFormat::VF_P3_N3 ) );
    floor->local().setTranslate( 0.125f * Vector3f::UNIT_Y );
    floor->getComponent< MaterialComponent >()->attachMaterial( material );
    scene->attachNode( floor );

    auto wall = crimild::alloc< Geometry >();
    wall->attachPrimitive( crimild::alloc< BoxPrimitive >( 20.0f, 30.0f, 0.25f, VertexFormat::VF_P3_N3 ) );
    wall->local().setTranslate( -10.0f * Vector3f::UNIT_Z );
    wall->getComponent< MaterialComponent >()->attachMaterial( material );
    scene->attachNode( wall );

    return scene;
}

SharedPointer< Node > buildPillar( const Vector3f &position )
{
    auto geometry = crimild::alloc< Geometry >();
	geometry->attachPrimitive( crimild::alloc< ConePrimitive >( Primitive::Type::TRIANGLES, 10.0f, 1.25f, VertexFormat::VF_P3_N3 ) );
    geometry->local().setTranslate( 5.0f * Vector3f::UNIT_Y + position );

    auto material = crimild::alloc< Material >();
    material->setAmbient( RGBAColorf::ONE );
    material->setDiffuse( COLOR_PILLAR );
    geometry->getComponent< MaterialComponent >()->attachMaterial( material );

    return geometry;
}

SharedPointer< Node > buildTeapot( void )
{
	auto geometry = crimild::alloc< Geometry >();
	geometry->attachPrimitive( crimild::alloc< NewellTeapotPrimitive >() );
    geometry->local().setScale( 0.25f );

    auto material = crimild::alloc< Material >();
    material->setAmbient( RGBAColorf::ONE );
    material->setDiffuse( COLOR_TEAPOT );
    geometry->getComponent< MaterialComponent >()->attachMaterial( material );

    return geometry;
}

SharedPointer< Node > loadScene( void )
{
    auto scene = crimild::alloc< Group >( "scene" );

    scene->attachNode( buildBackground() );

    auto pivot = crimild::alloc< Group >();
    pivot->attachNode( buildFloor() );
    pivot->attachNode( buildWall() );
    pivot->attachNode( buildTeapot() );
    pivot->attachNode( buildPillar( -8.0f * Vector3f::UNIT_X ) );
    pivot->attachNode( buildPillar( 8.0f * Vector3f::UNIT_X ) );
    pivot->attachNode( buildPillar( -8.0f * Vector3f::UNIT_Z ) );
    pivot->attachNode( buildPillar( 8.0f * Vector3f::UNIT_Z ) );
    pivot->attachComponent( crimild::alloc< RotationComponent >( Vector3f::UNIT_Y, 0.1f ) );
    scene->attachNode( pivot );

    return scene;
}

int main( int argc, char **argv )
{
	crimild::init();

    auto settings = crimild::alloc< Settings >( argc, argv );
    settings->set( "video.width", 1280 );
    settings->set( "video.height", 720 );
    settings->set( "video.show_frame_time", true );
	CRIMILD_SIMULATION_LIFETIME auto sim = crimild::alloc< sdl::SDLSimulation >( "Shadows", settings );

	auto scene = crimild::alloc< Group >();
    scene->attachNode( loadScene() );

    auto buildLight = []( crimild::Real32 yaw, crimild::Real32 pitch, crimild::Real32 roll ) {
        auto light = crimild::alloc< Light >( Light::Type::DIRECTIONAL );
        light->local().rotate().fromEulerAngles( yaw, pitch, roll );
        light->setColor( 0.5f * RGBAColorf::ONE );
        light->setCastShadows( true );
        light->getShadowMap()->setCullFaceState( CullFaceState::DISABLED );
        light->getShadowMap()->setMinBias( 0.005f );
        light->getShadowMap()->setMaxBias( 0.05f );
        return light;
    };

    scene->attachNode( buildLight( -1.0f, Numericf::HALF_PI, 0.0f ) );
    scene->attachNode( buildLight( -0.5f, Numericf::PI, 0.0f ) );

	auto camera = crimild::alloc< Camera >( 45.0f, 4.0f / 3.0f, 0.1f, 1024.0f );
	camera->local().setTranslate( 1.0f, 15.0f, 35.0f );
    camera->local().lookAt( Vector3f( 0.0f, 5.0f, 0.0 ), Vector3f( 0.0f, 1.0f, 0.0f ) );
	camera->setRenderPass( crimild::alloc< RenderGraphRenderPass >( createRenderGraph() ) );
	scene->attachNode( camera );

	sim->setScene( scene );

	sim->registerMessageHandler< crimild::messaging::KeyReleased >( [ camera ]( crimild::messaging::KeyReleased const &msg ) {
		switch ( msg.key ) {
			case CRIMILD_INPUT_KEY_Q:
				crimild::concurrency::sync_frame( [ camera ]() {
					std::cout << "Full" << std::endl;
					Renderer::getInstance()->setFrameBuffer( RenderPass::S_BUFFER_NAME, nullptr );
					auto renderGraph = createRenderGraph( false );
					camera->setRenderPass( crimild::alloc< RenderGraphRenderPass >( renderGraph ) );
				});
				break;
				
			case CRIMILD_INPUT_KEY_W:
				crimild::concurrency::sync_frame( [ camera ]() {
					std::cout << "Debug" << std::endl;
					Renderer::getInstance()->setFrameBuffer( RenderPass::S_BUFFER_NAME, nullptr );
					auto renderGraph = createRenderGraph( true );
					camera->setRenderPass( crimild::alloc< RenderGraphRenderPass >( renderGraph ) );
				});
				break;

			case CRIMILD_INPUT_KEY_A:
				crimild::concurrency::sync_frame( [ camera ]() {
					std::cout << "Legacy" << std::endl;
					Renderer::getInstance()->setFrameBuffer( RenderPass::S_BUFFER_NAME, nullptr );
					auto renderPass = crimild::alloc< CompositeRenderPass >();
					renderPass->attachRenderPass( crimild::alloc< ShadowRenderPass >() );
					renderPass->attachRenderPass( crimild::alloc< StandardRenderPass >() );
					camera->setRenderPass( renderPass );
				});
				break;
		}
	});
	return sim->run();
}

