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

#include <UI/UIFrame.hpp>
#include <UI/UICanvas.hpp>
#include <UI/UIBackground.hpp>

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
	canvas->attachComponent< UICanvas >( 100, 100 );
	canvas->attachComponent< UIBackground >( RGBAColorf( 1.0f, 1.0f, 0.0f, 1.0f ) );

	auto view = crimild::alloc< Group >();
	view->attachComponent< UIFrame >( Rectf( 50, 25, 50, 50 ) )
	    ->setZIndex( 1 );
	view->attachComponent< UIBackground >( RGBAColorf( 1.0f, 0.0f, 0.0f, 1.0f ));
	canvas->attachNode( view );

	auto view2 = crimild::alloc< Group >();
	view2->attachComponent< UIFrame >( Rectf( 25.0f, 25.0f, 25.0f, 25.0f ) )
	    ->setZIndex( 2 );
	view2->attachComponent< UIBackground >( RGBAColorf( 0.0f, 1.0f, 0.0f, 1.0f ) );
	view->attachNode( view2 );

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

