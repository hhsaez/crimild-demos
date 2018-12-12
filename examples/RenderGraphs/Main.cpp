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

namespace crimild {

	namespace rendergraph {

		class OpaquePass : public RenderGraphPass {
            CRIMILD_IMPLEMENT_RTTI( crimild::rendergraph::OpaquePass )
		public:
            OpaquePass( RenderGraph *graph, std::string name = "Render Opaque Objects" )
			    : RenderGraphPass( graph, name )
			{

			}
			
			virtual ~OpaquePass( void )
			{
				
			}
			
			void setDepthInput( RenderGraphAttachment *attachment ) { _depthInput = attachment; }
			RenderGraphAttachment *getDepthInput( void ) { return _depthInput; }
			
			void setColorOutput( RenderGraphAttachment *attachment ) { _colorOutput = attachment; }
			RenderGraphAttachment *getColorOutput( void ) { return _colorOutput; }
			
			virtual void setup( rendergraph::RenderGraph *graph ) override
			{
                _clearFlags = FrameBufferObject::ClearFlag::COLOR;
                _depthState = crimild::alloc< DepthState >( true, DepthState::CompareFunc::EQUAL, false );

				if ( _depthInput == nullptr ) {
					_depthInput = graph->createAttachment(
                        "Aux Depth Buffer",
						RenderGraphAttachment::Hint::FORMAT_DEPTH |
						RenderGraphAttachment::Hint::RENDER_ONLY );
                    _clearFlags = FrameBufferObject::ClearFlag::ALL;
                    _depthState = DepthState::ENABLED;
				}

                graph->read( this, { _depthInput } );
				graph->write( this, { _colorOutput } );

				_program = crimild::alloc< opengl::StandardShaderProgram >();
			}
			
			virtual void execute( RenderGraph *graph, Renderer *renderer, RenderQueue *renderQueue ) override
			{
                auto fbo = graph->createFBO( { _depthInput, _colorOutput } );
                fbo->setClearFlags( _clearFlags );

				CRIMILD_PROFILE( "Render Opaque Objects" )
				
                renderer->bindFrameBuffer( crimild::get_ptr( fbo ) );

				auto renderables = renderQueue->getRenderables( RenderQueue::RenderableType::OPAQUE );
				if ( renderables->size() == 0 ) {
					return;
				}

				auto program = crimild::get_ptr( _program );

				renderQueue->each( renderables, [ this, renderer, renderQueue, &program ]( RenderQueue::Renderable *renderable ) {
					auto material = crimild::get_ptr( renderable->material );
					
					renderer->bindProgram( program );
					
					auto projection = renderQueue->getProjectionMatrix();
					renderer->bindUniform( program->getStandardLocation( ShaderProgram::StandardLocation::PROJECTION_MATRIX_UNIFORM ), projection );
					
					auto view = renderQueue->getViewMatrix();
					renderer->bindUniform( program->getStandardLocation( ShaderProgram::StandardLocation::VIEW_MATRIX_UNIFORM ), view );
				
					renderer->bindUniform( program->getStandardLocation( ShaderProgram::StandardLocation::USE_SHADOW_MAP_UNIFORM ), false );
					
					renderQueue->each( [renderer, program]( Light *light, int ) {
						renderer->bindLight( program, light );
					});
					
					if ( material != nullptr ) {
						renderer->bindMaterial( program, material );
					}
				
					renderer->setDepthState( _depthState );

					renderer->drawGeometry( crimild::get_ptr( renderable->geometry ), program, renderable->modelTransform );
					
					if ( material != nullptr ) {
						renderer->unbindMaterial( program, material );
					}

					renderer->setDepthState( DepthState::ENABLED );
					
					renderQueue->each( [ renderer, program ]( Light *light, int ) {
						renderer->unbindLight( program, light );
					});
					
					renderer->unbindProgram( program );
				});
				
                renderer->unbindFrameBuffer( crimild::get_ptr( fbo ) );
			}
			
		private:
			SharedPointer< ShaderProgram > _program;
            crimild::Int8 _clearFlags;
            SharedPointer< DepthState > _depthState;

			RenderGraphAttachment *_depthInput = nullptr;
			RenderGraphAttachment *_colorOutput = nullptr;
		};

		class ColorTintPass : public RenderGraphPass {
			CRIMILD_IMPLEMENT_RTTI( crimild::rendergraph::ColorTintPass )

		public:
            ColorTintPass( RenderGraph *graph, std::string name = "Color Tint", const RGBAColorf &tint = RGBAColorf( 0.4392156863f, 0.2588235294f, 0.07843137255f, 1.0f ), crimild::Real32 value = 1.0f )
                : RenderGraphPass( graph, name )
			{
				_program = crimild::alloc< crimild::opengl::ColorTintShaderProgram >();
				_program->attachUniform( crimild::alloc< RGBAColorfUniform >( ColorTintImageEffect::COLOR_TINT_UNIFORM_TINT, tint ) );
				_program->attachUniform( crimild::alloc< FloatUniform >( ColorTintImageEffect::COLOR_TINT_UNIFORM_TINT_VALUE, value ) );
			}

			virtual ~ColorTintPass( void )
			{

			}

			void setInput( RenderGraphAttachment *attachment ) { _input = attachment; }
			RenderGraphAttachment *getInput( void ) { return _input; }

			void setOutput( RenderGraphAttachment *attachment ) { _output = attachment; }
			RenderGraphAttachment *getOutput( void ) { return _output; }

			virtual void setup( RenderGraph *graph ) override
			{
                graph->read( this, { _input } );
				graph->write( this, { _output } );
			}

			virtual void execute( RenderGraph *graph, Renderer *renderer, RenderQueue *renderQueue ) override
			{
				if ( _input == nullptr || _input->getTexture() == nullptr ) {
					return;
				}

                _gBuffer = graph->createFBO( { _output } );

				renderer->bindFrameBuffer( crimild::get_ptr( _gBuffer ) );
				
				auto program = crimild::get_ptr( _program );

				renderer->bindProgram( program );

				renderer->bindTexture(
					program->getStandardLocation( ShaderProgram::StandardLocation::COLOR_MAP_UNIFORM ),
					getInput()->getTexture() );

				renderer->drawScreenPrimitive( program );
				
				renderer->unbindTexture(
					program->getStandardLocation( ShaderProgram::StandardLocation::COLOR_MAP_UNIFORM ),
					getInput()->getTexture() );
				
				renderer->unbindProgram( program );

				renderer->unbindFrameBuffer( crimild::get_ptr( _gBuffer ) );
			}

		private:
			SharedPointer< FrameBufferObject > _gBuffer;
			SharedPointer< ShaderProgram > _program;
			
			RenderGraphAttachment *_input = nullptr;
			RenderGraphAttachment *_output = nullptr;
		};
		
		class FrameDebugPass : public RenderGraphPass {
			CRIMILD_IMPLEMENT_RTTI( crimild::rendergraph::FrameDebugPass )

		public:
            FrameDebugPass( RenderGraph *graph, std::string name = "Debug Frame" ) : RenderGraphPass( graph, name ) { }
			virtual ~FrameDebugPass( void ) { }

			void addInput( RenderGraphAttachment *input )
			{
				_inputs.add( input );
			}

			void setOutput( RenderGraphAttachment *attachment ) { _output = attachment; }
			RenderGraphAttachment *getOutput( void ) { return _output; }

			virtual void setup( RenderGraph *graph ) override
			{
                graph->read( this, _inputs );
				graph->write( this, { _output } );
			}

			virtual void execute( RenderGraph *graph, Renderer *renderer, RenderQueue *renderQueue ) override
			{
				if ( _inputs.size() == 0 ) {
					return;
				}

                _gBuffer = graph->createFBO( { _output } );
                _gBuffer->setClearColor( RGBAColorf( 1.0f, 1.0f, 1.0f, 1.0f ) );

				const static auto LAYOUT_12 = containers::Array< Rectf > {
					Rectf( 0.25f, 0.25f, 0.5f, 0.5f ),

					Rectf( 0.0f, 0.75f, 0.25f, 0.25f ),
					Rectf( 0.25f, 0.75f, 0.25f, 0.25f ),
					Rectf( 0.5f, 0.75f, 0.25f, 0.25f ),
					Rectf( 0.75f, 0.75f, 0.25f, 0.25f ),
					
					Rectf( 0.0f, 0.5f, 0.25f, 0.25f ),
					Rectf( 0.75f, 0.5f, 0.25f, 0.25f ),
					
					Rectf( 0.0f, 0.25f, 0.25f, 0.25f ),
					Rectf( 0.75f, 0.25f, 0.25f, 0.25f ),
					
					Rectf( 0.0f, 0.0f, 0.25f, 0.25f ),
					Rectf( 0.25f, 0.0f, 0.25f, 0.25f ),
					Rectf( 0.5f, 0.0f, 0.25f, 0.25f ),
					Rectf( 0.75f, 0.0f, 0.25f, 0.25f ),
				};

                const static auto LAYOUT_9 = containers::Array< Rectf > {
                    Rectf( 0.25f, 0.0f, 0.75f, 0.75f ),

                    Rectf( 0.005f, 0.755f, 0.24f, 0.24f ),
                    Rectf( 0.255f, 0.755f, 0.24f, 0.24f ),
                    Rectf( 0.505f, 0.755f, 0.24f, 0.24f ),
                    Rectf( 0.755f, 0.755f, 0.24f, 0.24f ),

                    Rectf( 0.005f, 0.505f, 0.24f, 0.24f ),
                    Rectf( 0.005f, 0.255f, 0.24f, 0.24f ),
                    Rectf( 0.005f, 0.005f, 0.24f, 0.24f ),
                };

				renderer->bindFrameBuffer( crimild::get_ptr( _gBuffer ) );

                auto layout = LAYOUT_9;

				_inputs.each( [ this, renderer, &layout ]( RenderGraphAttachment *input, crimild::Size idx ) {
					if ( idx < layout.size() ) {
						renderer->setViewport( layout[ idx ] );
						render( renderer, input->getTexture() );
					}
				});

				renderer->unbindFrameBuffer( crimild::get_ptr( _gBuffer ) );

				// reset viewport
				renderer->setViewport( Rectf( 0.0f, 0.0f, 1.0f, 1.0f ) );
			}

		private:
			void render( Renderer *renderer, Texture *texture )
			{
				auto program = renderer->getShaderProgram( Renderer::SHADER_PROGRAM_SCREEN_TEXTURE );

				renderer->bindProgram( program );

				renderer->bindTexture(
					program->getStandardLocation( ShaderProgram::StandardLocation::COLOR_MAP_UNIFORM ),
					texture );

				renderer->drawScreenPrimitive( program );
				
				renderer->unbindTexture(
					program->getStandardLocation( ShaderProgram::StandardLocation::COLOR_MAP_UNIFORM ),
					texture );
				
				renderer->unbindProgram( program );				
			}

		private:
			SharedPointer< FrameBufferObject > _gBuffer;

			containers::Array< RenderGraphAttachment * > _inputs;

			RenderGraphAttachment *_output = nullptr;
		};

		class HDRSceneRenderPass : public RenderGraphPass {
			CRIMILD_IMPLEMENT_RTTI( crimild::rendergraph::HDRSceneRenderPass )
			
		public:
			HDRSceneRenderPass( RenderGraph *graph )
			    : RenderGraphPass( graph, "HDR Scene" )
			{
				_depthOutput = graph->createAttachment( getName() + " - Depth", RenderGraphAttachment::Hint::FORMAT_DEPTH_HDR );
				_normalOutput = graph->createAttachment( getName() + " - Normal", RenderGraphAttachment::Hint::FORMAT_RGBA );
				_opaqueOutput = graph->createAttachment( getName() + " - Opaque", RenderGraphAttachment::Hint::FORMAT_RGBA );
				_translucentOutput = graph->createAttachment( getName() + " - Translucent", RenderGraphAttachment::Hint::FORMAT_RGBA );
				_colorOutput = graph->createAttachment( getName() + " - Color", RenderGraphAttachment::Hint::FORMAT_RGBA );
			}

			virtual ~HDRSceneRenderPass( void )
			{
				
			}

			void setDepthOutput( RenderGraphAttachment *output ) { _depthOutput = output; };
			RenderGraphAttachment *getDepthOutput( void ) { return _depthOutput; }

			void setNormalOutput( RenderGraphAttachment *output ) { _normalOutput = output; };
            RenderGraphAttachment *getNormalOutput( void ) { return _normalOutput; }

			void setOpaqueOutput( RenderGraphAttachment *output ) { _opaqueOutput = output; };
            RenderGraphAttachment *getOpaqueOutput( void ) { return _opaqueOutput; }

			void setTranslucentOutput( RenderGraphAttachment *output ) { _translucentOutput = output; };
            RenderGraphAttachment *getTranslucentOutput( void ) { return _translucentOutput; }

			void setColorOutput( RenderGraphAttachment *output ) { _colorOutput = output; };
            RenderGraphAttachment *getColorOutput( void ) { return _colorOutput; }

        private:
            RenderGraphAttachment *_depthOutput = nullptr;
            RenderGraphAttachment *_normalOutput = nullptr;
            RenderGraphAttachment *_opaqueOutput = nullptr;
            RenderGraphAttachment *_translucentOutput = nullptr;
            RenderGraphAttachment *_colorOutput = nullptr;

		public:
			
			virtual void setup( RenderGraph *graph ) override
			{
                auto depthPass = graph->createPass< passes::DepthPass >();
                auto opaquePass = graph->createPass< OpaquePass >();
				auto translucentTypes = containers::Array< RenderQueue::RenderableType > {
					RenderQueue::RenderableType::OPAQUE_CUSTOM,
					RenderQueue::RenderableType::TRANSLUCENT,
				};
				auto translucentPass = graph->createPass< passes::ForwardLightingPass >( translucentTypes );
				
                auto colorPass = graph->createPass< passes::BlendPass >();

				depthPass->setDepthOutput( _depthOutput );
				depthPass->setNormalOutput( _normalOutput );

				opaquePass->setDepthInput( _depthOutput );
                opaquePass->setColorOutput( _opaqueOutput );

                translucentPass->setDepthInput( _depthOutput );
                translucentPass->setColorOutput( _translucentOutput );

                colorPass->addInput( _opaqueOutput );
                colorPass->addInput( _translucentOutput );
                colorPass->setOutput( _colorOutput );
			}

			virtual void execute( RenderGraph *graph, Renderer *renderer, RenderQueue *renderQueue ) override
			{
				// do nothing
			}
		};

	}

}

using namespace crimild;
using namespace crimild::rendergraph;
using namespace crimild::rendergraph::passes;

SharedPointer< RenderGraph > createForwardRenderGraph( crimild::Bool enableDebug )
{
	auto renderGraph = crimild::alloc< RenderGraph >();

    auto depthPass = renderGraph->createPass< DepthPass >( );
	auto types = containers::Array< RenderQueue::RenderableType > {
		RenderQueue::RenderableType::OPAQUE,
		RenderQueue::RenderableType::TRANSLUCENT,
	};
	auto forwardPass = renderGraph->createPass< ForwardLightingPass >( types );
    auto sepiaPass = renderGraph->createPass< ColorTintPass >();
    auto depthToColorPass = renderGraph->createPass< DepthToRGBPass >();
    auto debugPass = renderGraph->createPass< FrameDebugPass >();

    auto sepiaBuffer = renderGraph->createAttachment( "Sepia", RenderGraphAttachment::Hint::FORMAT_RGBA );
    auto depthColorBuffer = renderGraph->createAttachment( "Depth RGB", RenderGraphAttachment::Hint::FORMAT_RGBA );
    auto debugBuffer = renderGraph->createAttachment( "Debug", RenderGraphAttachment::Hint::FORMAT_RGBA );

	forwardPass->setDepthInput( depthPass->getDepthOutput() );

    sepiaPass->setInput( forwardPass->getColorOutput() );
    sepiaPass->setOutput( sepiaBuffer );

    depthToColorPass->setInput( depthPass->getDepthOutput() );
    depthToColorPass->setOutput( depthColorBuffer );

    debugPass->addInput( sepiaBuffer );
	debugPass->addInput( depthToColorPass->getOutput() );
	debugPass->addInput( depthPass->getNormalOutput() );
    debugPass->addInput( forwardPass->getColorOutput() );
    debugPass->setOutput( debugBuffer );

    renderGraph->setOutput( enableDebug ? debugBuffer : sepiaBuffer );

	return renderGraph;
}

SharedPointer< RenderGraph > createRenderGraph( crimild::Bool useDebugPass )
{
	auto renderGraph = crimild::alloc< RenderGraph >();

	auto scenePass = renderGraph->createPass< HDRSceneRenderPass >();
    auto depthRGBPass = renderGraph->createPass< DepthToRGBPass >();
    auto debugPass = renderGraph->createPass< FrameDebugPass >();
    auto sepiaPass = renderGraph->createPass< ColorTintPass >();
	auto screenPass = renderGraph->createPass< ScreenPass >();
    auto compositionPass = renderGraph->createPass< BlendPass >( AlphaState::ENABLED );

    auto depthRGBBuffer = renderGraph->createAttachment( "Depth RGB", RenderGraphAttachment::Hint::FORMAT_RGBA );
    auto debugBuffer = renderGraph->createAttachment( "Debug", RenderGraphAttachment::Hint::FORMAT_RGBA );
    auto sepiaBuffer = renderGraph->createAttachment( "Sepia", RenderGraphAttachment::Hint::FORMAT_RGBA );
    auto frameBuffer = renderGraph->createAttachment( "Scene + Screen", RenderGraphAttachment::Hint::FORMAT_RGBA );

#if 0
	auto colorBuffer = renderGraph->createAttachment( "Color + Translucent", RenderGraphAttachment::Hint::FORMAT_RGBA | RenderGraphAttachment::Hint::SIZE_SCREEN_10 );
	scenePass->setColorOutput( colorBuffer );
#endif
	
    sepiaPass->setInput( scenePass->getColorOutput() );
    sepiaPass->setOutput( sepiaBuffer );

	compositionPass->addInput( sepiaBuffer );
	compositionPass->addInput( screenPass->getOutput() );
	compositionPass->setOutput( frameBuffer );

    depthRGBPass->setInput( scenePass->getDepthOutput() );
    depthRGBPass->setOutput( depthRGBBuffer );

	debugPass->addInput( frameBuffer );
    debugPass->addInput( depthRGBBuffer );
    debugPass->addInput( scenePass->getNormalOutput() );
    debugPass->addInput( scenePass->getOpaqueOutput() );
    debugPass->addInput( scenePass->getTranslucentOutput() );
    debugPass->addInput( scenePass->getColorOutput() );
    debugPass->addInput( sepiaBuffer );
	debugPass->addInput( screenPass->getOutput() );
    debugPass->setOutput( debugBuffer );

	renderGraph->setOutput( useDebugPass ? debugBuffer : frameBuffer );

	return renderGraph;
}

SharedPointer< Node > createTeapots( void )
{
	auto scene = crimild::alloc< Group >();

	auto teapot = []( const Vector3f &position, const RGBAColorf &color, crimild::Real32 scale = 1.0f, crimild::Bool isOccluder = false, crimild::Bool renderOnScreen = false ) -> SharedPointer< Node > {
		auto geometry = crimild::alloc< Geometry >();
		auto primitive = crimild::alloc< NewellTeapotPrimitive >();
		geometry->attachPrimitive( primitive );
		geometry->perform( UpdateWorldState() );
		geometry->local().setScale( scale * 1.0f / geometry->getWorldBound()->getRadius() );
		geometry->local().setTranslate( position );

		auto m = crimild::alloc< Material >();
		m->setDiffuse( color );
		if ( color.a() < 1.0f ) m->setAlphaState( AlphaState::ENABLED );
		if ( isOccluder ) m->setColorMaskState( ColorMaskState::DISABLED );
        geometry->getComponent< MaterialComponent >()->attachMaterial( m );

		geometry->getComponent< RenderStateComponent >()->setRenderOnScreen( renderOnScreen );

		return geometry;
	};

	scene->attachNode( teapot( Vector3f( -10.0f, 0.0f, 10.0f ), RGBAColorf( 1.0f, 1.0f, 1.0f, 1.0f ), 10.0f ) );
	scene->attachNode( teapot( Vector3f( 10.0f, 0.0f, 10.0f ), RGBAColorf( 1.0f, 1.0f, 1.0f, 1.0f ), 5.0f ) );
	scene->attachNode( teapot( Vector3f( -10.0f, 0.0f, -10.0f ), RGBAColorf( 1.0f, 1.0f, 1.0f, 1.0f ), 6.0f ) );
	scene->attachNode( teapot( Vector3f( 10.0f, 0.0f, -10.0f ), RGBAColorf( 1.0f, 1.0f, 1.0f, 0.75f ), 8.0f ) );
    scene->attachNode( teapot( Vector3f( 0.0f, 0.0f, 0.0f ), RGBAColorf( 1.0f, 1.0f, 1.0f, 1.0f ), 10.0f, true ) );
	scene->attachNode( teapot( Vector3f( 0.0f, -0.8f, 0.0f ), RGBAColorf( 0.0f, 1.0f, 0.0f, 1.0f ), 0.1f, false, true ) );

	return scene;
}

SharedPointer< Node > buildLight( const Quaternion4f &rotation, const RGBAColorf &color, float major, float minor, float speed )
{
	auto orbitingLight = crimild::alloc< Group >();

	auto primitive = crimild::alloc< SpherePrimitive >( 0.5f );
	auto geometry = crimild::alloc< Geometry >();
	geometry->attachPrimitive( primitive );

	auto material = crimild::alloc< Material >();
	material->setDiffuse( color );
   	material->setProgram( AssetManager::getInstance()->get< ShaderProgram >( Renderer::SHADER_PROGRAM_UNLIT_DIFFUSE ) );
    material->setAlphaState( AlphaState::ENABLED );
	geometry->getComponent< MaterialComponent >()->attachMaterial( material );

	orbitingLight->attachNode( geometry );

	auto light = crimild::alloc< Light >();
	light->setColor( color );
    light->setAmbient( 0.1f * color );
	orbitingLight->attachNode( light );

	auto orbitComponent = crimild::alloc< OrbitComponent >( 0.0f, 0.0f, major, minor, speed );
	orbitingLight->attachComponent( orbitComponent );

	auto group = crimild::alloc< Group >();
	group->attachNode( orbitingLight );
	group->local().setRotate( rotation );

	return group;
}

SharedPointer< Node > buildUI( void )
{
	auto ui = crimild::alloc< Group >();

	auto fontFile = FileSystem::getInstance().pathForResource( "assets/fonts/Verdana.txt" );
	auto font = crimild::alloc< Font >( fontFile );
	auto text = crimild::alloc< Text >();
	text->setFont( font );
	text->setSize( 0.1 );
	text->setText( "Render Graphs are Working :)" );
	text->setHorizontalAlignment( Text::HorizontalAlignment::CENTER );
	text->setTextColor( RGBAColorf( 0.0f, 1.0f, 0.0f, 1.0f ) );
	text->getNodeAt( 0 )->getComponent< RenderStateComponent >()->setRenderOnScreen( true );
	text->local().setTranslate( 0.0f, -0.9f, 0.0f );
	ui->attachNode( text );

	return ui;
}

int main( int argc, char **argv )
{
    auto settings = crimild::alloc< Settings >( argc, argv );
    settings->set( "video.show_frame_time", true );
    auto sim = crimild::alloc< sdl::SDLSimulation >( "Render Graphs", settings );

    auto scene = crimild::alloc< Group >();

	auto teapots = createTeapots();
	teapots->attachComponent< RotationComponent >( Vector3f::UNIT_Y, 0.05f );
	scene->attachNode( teapots );

	scene->attachNode( buildUI() );

	scene->attachNode( buildLight( 
		Quaternion4f::createFromAxisAngle( Vector3f( 0.0f, 1.0f, 1.0 ).getNormalized(), Numericf::HALF_PI ), 
		RGBAColorf( 1.0f, 0.0f, 0.0f, 1.0f ),
		-20.0f, 20.25f, 0.95f ).get() );

	scene->attachNode( buildLight( 
		Quaternion4f::createFromAxisAngle( Vector3f( 0.0f, 1.0f, -1.0 ).getNormalized(), -Numericf::HALF_PI ), 
		RGBAColorf( 0.0f, 1.0f, 0.0f, 1.0f ), 
		20.0f, 20.25f, -0.75f ).get() );

	scene->attachNode( buildLight( 
		Quaternion4f::createFromAxisAngle( Vector3f( 1.0f, 1.0f, 0.0 ).getNormalized(), Numericf::HALF_PI ),
		RGBAColorf( 0.0f, 0.0f, 1.0f, 1.0f ), 
		20.25f, 20.0f, -0.85f ).get() );

	auto camera = crimild::alloc< Camera >( 45.0f, 4.0f / 3.0f, 0.1f, 1024.0f );
	camera->local().setTranslate( 0.0f, 5.0f, 30.0f );
	camera->local().lookAt( Vector3f( 0.0f, 3.0f, 0.0f ) );
    auto renderGraph = createRenderGraph( false );
    camera->setRenderPass( crimild::alloc< RenderGraphRenderPass >( renderGraph ) );
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
                    std::cout << "Full (Debug)" << std::endl;
                    Renderer::getInstance()->setFrameBuffer( RenderPass::S_BUFFER_NAME, nullptr );
                    auto renderGraph = createRenderGraph( true );
                    camera->setRenderPass( crimild::alloc< RenderGraphRenderPass >( renderGraph ) );
                });
                break;

            case CRIMILD_INPUT_KEY_E:
                crimild::concurrency::sync_frame( [ camera ]() {
                    std::cout << "Forward" << std::endl;
                    Renderer::getInstance()->setFrameBuffer( RenderPass::S_BUFFER_NAME, nullptr );
                    auto renderGraph = createForwardRenderGraph( false );
                    camera->setRenderPass( crimild::alloc< RenderGraphRenderPass >( renderGraph ) );
                });
                break;

            case CRIMILD_INPUT_KEY_R:
                crimild::concurrency::sync_frame( [ camera ]() {
                    std::cout << "Forward (Debug)" << std::endl;
                    Renderer::getInstance()->setFrameBuffer( RenderPass::S_BUFFER_NAME, nullptr );
                    auto renderGraph = createForwardRenderGraph( true );
                    camera->setRenderPass( crimild::alloc< RenderGraphRenderPass >( renderGraph ) );
                });
                break;

			case CRIMILD_INPUT_KEY_A:
				crimild::concurrency::sync_frame( [ camera ]() {
					std::cout << "RenderPass (post)" << std::endl;
					Renderer::getInstance()->setFrameBuffer( RenderPass::S_BUFFER_NAME, nullptr );
					camera->setRenderPass( crimild::alloc< PostRenderPass >( crimild::alloc< StandardRenderPass >() ) );
					auto sepiaToneEffect = crimild::alloc< ColorTintImageEffect >( ColorTintImageEffect::TINT_SEPIA );
					camera->getRenderPass()->getImageEffects().add( sepiaToneEffect );
				});
				break;

			case CRIMILD_INPUT_KEY_S:
				crimild::concurrency::sync_frame( [ camera ]() {
					std::cout << "RenderPass (no post)" << std::endl;
					Renderer::getInstance()->setFrameBuffer( RenderPass::S_BUFFER_NAME, nullptr );
					camera->setRenderPass( crimild::alloc< StandardRenderPass >() );
				});
				break;
		}
	});
	
	return sim->run();
}

