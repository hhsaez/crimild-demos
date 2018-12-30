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

namespace crimild {

	namespace rendergraph {

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

		class HDRSceneRenderPass : public RenderGraphPass {
			CRIMILD_IMPLEMENT_RTTI( crimild::rendergraph::HDRSceneRenderPass )
			
		public:
			HDRSceneRenderPass( RenderGraph *graph )
			    : RenderGraphPass( graph, "HDR Scene" )
			{
				_depthOutput = graph->createAttachment( getName() + " - Depth", RenderGraphAttachment::Hint::FORMAT_DEPTH_HDR );
				_normalOutput = graph->createAttachment( getName() + " - Normal", RenderGraphAttachment::Hint::FORMAT_RGBA_HDR );
				_lightingADOutput = graph->createAttachment( getName() + " - Lighting Ambient + Diffuse", RenderGraphAttachment::Hint::FORMAT_RGBA );
				_lightingSOutput = graph->createAttachment( getName() + " - Lighting Specular", RenderGraphAttachment::Hint::FORMAT_RGBA );
				_opaqueOutput = graph->createAttachment( getName() + " - Opaque", RenderGraphAttachment::Hint::FORMAT_RGBA );
				_opaqueLitOutput = graph->createAttachment( getName() + " - Opaque Lit", RenderGraphAttachment::Hint::FORMAT_RGBA );
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

			void setLightingADOutput( RenderGraphAttachment *output ) { _lightingADOutput = output; };
            RenderGraphAttachment *getLightingADOutput( void ) { return _lightingADOutput; }

			void setLightingSOutput( RenderGraphAttachment *output ) { _lightingSOutput = output; };
            RenderGraphAttachment *getLightingSOutput( void ) { return _lightingSOutput; }

			void setOpaqueOutput( RenderGraphAttachment *output ) { _opaqueOutput = output; };
            RenderGraphAttachment *getOpaqueOutput( void ) { return _opaqueOutput; }

			void setOpaqueLitOutput( RenderGraphAttachment *output ) { _opaqueLitOutput = output; };
            RenderGraphAttachment *getOpaqueLitOutput( void ) { return _opaqueLitOutput; }

			void setTranslucentOutput( RenderGraphAttachment *output ) { _translucentOutput = output; };
            RenderGraphAttachment *getTranslucentOutput( void ) { return _translucentOutput; }

			void setColorOutput( RenderGraphAttachment *output ) { _colorOutput = output; };
            RenderGraphAttachment *getColorOutput( void ) { return _colorOutput; }

        private:
            RenderGraphAttachment *_depthOutput = nullptr;
            RenderGraphAttachment *_normalOutput = nullptr;
            RenderGraphAttachment *_lightingADOutput = nullptr;
            RenderGraphAttachment *_lightingSOutput = nullptr;
            RenderGraphAttachment *_opaqueOutput = nullptr;
            RenderGraphAttachment *_opaqueLitOutput = nullptr;
            RenderGraphAttachment *_translucentOutput = nullptr;
            RenderGraphAttachment *_colorOutput = nullptr;

		public:
			
			virtual void setup( RenderGraph *graph ) override
			{
                auto depthPass = graph->createPass< passes::DepthPass >();
				auto lightingPass = graph->createPass< passes::LightAccumulationPass >();
                auto opaquePass = graph->createPass< passes::OpaquePass >();
				auto opaqueLitPass = graph->createPass< passes::BlendPass >( AlphaState::ENABLED_MULTIPLY_BLEND );
				auto translucentTypes = containers::Array< RenderQueue::RenderableType > {
					RenderQueue::RenderableType::OPAQUE_CUSTOM,
					RenderQueue::RenderableType::TRANSLUCENT,
				};
				auto translucentPass = graph->createPass< passes::ForwardLightingPass >( translucentTypes );
				
                auto colorPass = graph->createPass< passes::BlendPass >();

				depthPass->setDepthOutput( _depthOutput );
				depthPass->setNormalOutput( _normalOutput );

				lightingPass->setDepthInput( _depthOutput );
				lightingPass->setNormalInput( _normalOutput );
				lightingPass->setAmbientDiffuseOutput( _lightingADOutput );
				lightingPass->setSpecularOutput( _lightingSOutput );

				opaquePass->setDepthInput( _depthOutput );
                opaquePass->setColorOutput( _opaqueOutput );

				opaqueLitPass->addInput( _opaqueOutput );
				opaqueLitPass->addInput( _lightingADOutput );
				opaqueLitPass->setOutput( _opaqueLitOutput );

                translucentPass->setDepthInput( _depthOutput );
                translucentPass->setColorOutput( _translucentOutput );

                colorPass->addInput( _opaqueLitOutput );
                colorPass->addInput( _lightingSOutput );
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

    sepiaPass->setInput( scenePass->getColorOutput() );
    sepiaPass->setOutput( sepiaBuffer );

	compositionPass->addInput( sepiaBuffer );
	compositionPass->addInput( screenPass->getOutput() );
	compositionPass->setOutput( frameBuffer );

    depthRGBPass->setInput( scenePass->getDepthOutput() );
    depthRGBPass->setOutput( depthRGBBuffer );

	//debugPass->addInput( frameBuffer );
    //debugPass->addInput( depthRGBBuffer );
    debugPass->addInput( scenePass->getNormalOutput() );
    debugPass->addInput( scenePass->getLightingADOutput() );
    debugPass->addInput( scenePass->getLightingSOutput() );
    //debugPass->addInput( scenePass->getOpaqueOutput() );
    //debugPass->addInput( scenePass->getOpaqueLitOutput() );
    //debugPass->addInput( scenePass->getTranslucentOutput() );
    debugPass->addInput( scenePass->getColorOutput() );
    //debugPass->addInput( sepiaBuffer );
	//debugPass->addInput( screenPass->getOutput() );
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

	scene->attachNode( teapot( Vector3f( -10.0f, 0.0f, 10.0f ), RGBAColorf( 1.0f, 0.0f, 0.0f, 1.0f ), 10.0f ) );
	scene->attachNode( teapot( Vector3f( 10.0f, 0.0f, 10.0f ), RGBAColorf( 0.0f, 1.0f, 0.0f, 1.0f ), 5.0f ) );
	scene->attachNode( teapot( Vector3f( -10.0f, 0.0f, -10.0f ), RGBAColorf( 0.0f, 0.0f, 1.0f, 1.0f ), 6.0f ) );
	scene->attachNode( teapot( Vector3f( 10.0f, 0.0f, -10.0f ), RGBAColorf( 1.0f, 1.0f, 1.0f, 0.75f ), 8.0f ) );
    scene->attachNode( teapot( Vector3f( 0.0f, 0.0f, 0.0f ), RGBAColorf( 1.0f, 1.0f, 1.0f, 1.0f ), 10.0f, true ) );
	scene->attachNode( teapot( Vector3f( 0.0f, -0.8f, 0.0f ), RGBAColorf( 0.0f, 1.0f, 0.0f, 1.0f ), 0.1f, false, true ) );

	return scene;
}

SharedPointer< Node > buildAmbientLight( const RGBAColorf &color )
{
	auto light = crimild::alloc< Light >( Light::Type::AMBIENT );
	light->setAmbient( color );
	return light;
}

SharedPointer< Node > buildDirectionalLight( const RGBAColorf &color, const Vector3f &angles )
{
	auto light = crimild::alloc< Light >( Light::Type::DIRECTIONAL );
	light->setColor( color );
	light->local().rotate().fromEulerAngles( angles.x(), angles.y(), angles.z() );
	return light;
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
    light->setAmbient( RGBAColorf::ZERO );
	light->local().setScale( 5.0f );
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

	scene->attachNode( buildAmbientLight( RGBAColorf( 0.1f, 0.1f, 0.1f, 1.0f ) ) );
	scene->attachNode( buildDirectionalLight( RGBAColorf( 1.0f, 0.0f, 1.0f, 1.0f ), Vector3f( -0.25f * Numericf::PI, Numericf::HALF_PI, 0.0f ) ) );
	scene->attachNode( buildDirectionalLight( RGBAColorf( 0.0f, 1.0f, 1.0f, 1.0f ), Vector3f( 0.25f * Numericf::PI, -Numericf::HALF_PI, 0.0f ) ) );
	/*
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
	*/

	auto camera = crimild::alloc< Camera >( 45.0f, 4.0f / 3.0f, 0.1f, 1024.0f );
	camera->local().setTranslate( 0.0f, 5.0f, 30.0f );
	camera->local().lookAt( Vector3f( 0.0f, 3.0f, 0.0f ) );
	//camera->attachComponent< FreeLookCameraComponent >();
    auto renderGraph = createRenderGraph( true );
    camera->setRenderPass( crimild::alloc< RenderGraphRenderPass >( renderGraph ) );
	scene->attachNode( camera );
    
    sim->setScene( scene );

	sim->registerMessageHandler< crimild::messaging::KeyReleased >( [ camera, teapots ]( crimild::messaging::KeyReleased const &msg ) {
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

			case CRIMILD_INPUT_KEY_SPACE:
				if ( auto rot = teapots->getComponent< RotationComponent >() ) {
					rot->setEnabled( !rot->isEnabled() );
				}
				break;
		}
	});
	
	return sim->run();
}

