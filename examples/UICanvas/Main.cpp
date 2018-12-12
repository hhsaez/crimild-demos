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

#include "UI/UIFrame.hpp"
#include "UI/UICanvas.hpp"
#include "UI/UIBackground.hpp"
#include "UI/UIFrameConstraint.hpp"
#include "UI/UIFrameConstraintMaker.hpp"

using namespace crimild;
using namespace crimild::rendergraph;
using namespace crimild::rendergraph::passes;
using namespace crimild::ui;

SharedPointer< RenderGraph > createRenderGraph( crimild::Bool enableDebug )
{
	auto renderGraph = crimild::alloc< RenderGraph >();

	auto types = containers::Array< RenderQueue::RenderableType > {
		RenderQueue::RenderableType::OPAQUE,
		RenderQueue::RenderableType::OPAQUE_CUSTOM,
		RenderQueue::RenderableType::TRANSLUCENT,
		RenderQueue::RenderableType::TRANSLUCENT_CUSTOM,
	};
	auto forwardPass = renderGraph->createPass< ForwardLightingPass >( types );
	auto screenPass = renderGraph->createPass< ScreenPass >();
	auto compositionPass = renderGraph->createPass< BlendPass >( AlphaState::ENABLED );

	auto frameBuffer = renderGraph->createAttachment( "Frame", RenderGraphAttachment::Hint::FORMAT_RGBA );

	compositionPass->addInput( forwardPass->getColorOutput() );
	compositionPass->addInput( screenPass->getOutput() );
	compositionPass->setOutput( frameBuffer );

	renderGraph->setOutput( frameBuffer );

	return renderGraph;
}

SharedPointer< Node > buildUI( void )
{
	auto canvas = crimild::alloc< Group >();
	canvas->attachComponent< UICanvas >( 640, 480 );
	canvas->attachComponent< UIBackground >( RGBAColorf( 1.0f, 1.0f, 1.0f, 1.0f ) );

	auto view1 = crimild::alloc< Group >();
	view1->attachComponent< UIFrame >()->pin()->top( canvas )->left( canvas )->size( 200, 400 )->margin( 10.0f );
	view1->attachComponent< UIBackground >( RGBAColorf( 1.0f, 0.0f, 0.0f, 1.0f ) );
	canvas->attachNode( view1 );

	auto view2 = crimild::alloc< Group >();
	view2->attachComponent< UIFrame >()->pin()->fillParent()->margin( 20 );
	view2->attachComponent< UIBackground >( RGBAColorf( 0.0f, 1.0f, 0.0f, 1.0f ) );
	view1->attachNode( view2 );

	auto view3 = crimild::alloc< Group >();
	view3->attachComponent< UIFrame >()->pin()->size( 200, 300 )->after( view1 )->centerY( view1 )->marginLeft( 10 );
	view3->attachComponent< UIBackground >( RGBAColorf( 0.0f, 0.0f, 1.0f, 1.0f ) );
	canvas->attachNode( view3 );

	auto view4 = crimild::alloc< Group >();
	view4->attachComponent< UIFrame >()->pin()->size( 50, 30 )->below( view3 )->centerX( view3 )->marginTop( 20 );
	view4->attachComponent< UIBackground >( RGBAColorf( 0.0f, 1.0f, 1.0f, 1.0f ) );
	canvas->attachNode( view4 );

	auto view5 = crimild::alloc< Group >();
	view5->attachComponent< UIFrame >()->pin()->after( view3 )->top( view3 )->right( canvas )->height( 250 )->margin( 0, 10, 0, 10 );
	view5->attachComponent< UIBackground >( RGBAColorf( 1.0f, 0.0f, 1.0f, 1.0f ) );
	canvas->attachNode( view5 );

	auto view6 = crimild::alloc< Group >();
	view6->attachComponent< UIFrame >()->pin()->after( view3 )->below( view5 )->bottom( view4 )->right( canvas )->margin( 10, 5, 0, 5 );
	view6->attachComponent< UIBackground >( RGBAColorf( 1.0f, 0.5f, 0.0f, 1.0f ) );
	canvas->attachNode( view6 );

	auto view7 = crimild::alloc< Group >();
	view7->attachComponent< UIFrame >()->pin()->size( 300, 50 )->centerX( view6 )->right()->bottom();
	view7->attachComponent< UIBackground >( RGBAColorf( 1.0f, 1.0f, 0.0f, 1.0f ) );
	canvas->attachNode( view7 );

	canvas->perform( ApplyToGeometries( []( Geometry *geo ) {
		geo->getComponent< RenderStateComponent >()->setRenderOnScreen( true );
	}));
	
	return canvas;
}

int main( int argc, char **argv )
{
    auto settings = crimild::alloc< Settings >( argc, argv );
    settings->set( "video.show_frame_time", true );
    auto sim = crimild::alloc< sdl::SDLSimulation >( "UICanvas", settings );

    auto scene = crimild::alloc< Group >();

	scene->attachNode( buildUI() );

	auto camera = crimild::alloc< Camera >( 45.0f, 4.0f / 3.0f, 0.1f, 1024.0f );
	camera->local().setTranslate( -2.0f, 2.0f, 2.0f );
	camera->local().lookAt( Vector3f::ZERO );
    auto renderGraph = createRenderGraph( false );
    camera->setRenderPass( crimild::alloc< RenderGraphRenderPass >( renderGraph ) );
	scene->attachNode( camera );
    
    sim->setScene( scene );

	sim->registerMessageHandler< crimild::messaging::KeyReleased >( [ scene ]( crimild::messaging::KeyReleased const &msg ) {
		switch ( msg.key ) {
			case CRIMILD_INPUT_KEY_Q:
				scene->perform( ApplyToGeometries( []( Geometry *geo ) {
					geo->getComponent< RenderStateComponent >()->setRenderOnScreen( true );
				}));
				break;

			case CRIMILD_INPUT_KEY_W:
				scene->perform( ApplyToGeometries( []( Geometry *geo ) {
					geo->getComponent< RenderStateComponent >()->setRenderOnScreen( false );
				}));
				break;
		}
	});
	return sim->run();
}

