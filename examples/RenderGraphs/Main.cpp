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
#include <Crimild_OpenGL.hpp>

#include "Rendering/RenderGraph/RenderGraph.hpp"
#include "Rendering/RenderGraph/RenderGraphPass.hpp"
#include "Rendering/RenderGraph/RenderGraphAttachment.hpp"

namespace crimild {

	namespace rendergraph {

		class DepthPass : public RenderGraphPass {
            CRIMILD_IMPLEMENT_RTTI( crimild::rendergraph::DepthPass )
		public:
            DepthPass( std::string name = "Depth Pass" )
                : RenderGraphPass( name )
			{

			}
			
			virtual ~DepthPass( void )
			{
				
			}
			
			void setDepthOutput( RenderGraphAttachment *attachment ) { _depthOutput = attachment; }
			RenderGraphAttachment *getDepthOutput( void ) { return _depthOutput; }
			
			void setNormalOutput( RenderGraphAttachment *attachment ) { _normalOutput = attachment; }
			RenderGraphAttachment *getNormalOutput( void ) { return _normalOutput; }
			
			virtual void setup( rendergraph::RenderGraph *graph ) override
			{
				graph->write( this, { _depthOutput, _normalOutput } );

				const char *normal_vs = R"(
                    CRIMILD_GLSL_ATTRIBUTE vec3 aPosition;
                    CRIMILD_GLSL_ATTRIBUTE vec3 aNormal;

                    uniform mat4 uPMatrix;
                    uniform mat4 uVMatrix;
                    uniform mat4 uMMatrix;

                    CRIMILD_GLSL_VARYING_OUT vec3 vScreenNormal;

                    void main()
                    {
		vec4 vWorldVertex = uMMatrix * vec4( aPosition, 1.0 );
    vec4 viewVertex = uVMatrix * vWorldVertex;
    gl_Position = uPMatrix * viewVertex;
                        //CRIMILD_GLSL_VERTEX_OUTPUT = uPMatrix * uVMatrix * uMMatrix * vec4(aPosition, 1.0);
                        vScreenNormal = normalize( uVMatrix * uMMatrix * vec4( aNormal, 0.0 ) ).xyz;
                    }
                )";

                const char *normal_fs = R"(
                    CRIMILD_GLSL_PRECISION_FLOAT_HIGH

                    CRIMILD_GLSL_VARYING_IN vec3 vScreenNormal;

                    CRIMILD_GLSL_DECLARE_FRAGMENT_OUTPUT

                    void main( void )
                    {
                        vec3 normal = normalize( vScreenNormal );
                        CRIMILD_GLSL_FRAGMENT_OUTPUT = vec4( normal, 1.0 );
                    }
                )";

                _program = crimild::alloc< ShaderProgram >();
                _program->setVertexShader( opengl::OpenGLUtils::getVertexShaderInstance( normal_vs ) );
                _program->setFragmentShader( opengl::OpenGLUtils::getFragmentShaderInstance( normal_fs ) );
                _program->registerStandardLocation( ShaderLocation::Type::ATTRIBUTE, ShaderProgram::StandardLocation::POSITION_ATTRIBUTE, "aPosition" );
                _program->registerStandardLocation( ShaderLocation::Type::ATTRIBUTE, ShaderProgram::StandardLocation::NORMAL_ATTRIBUTE, "aNormal" );
                _program->registerStandardLocation( ShaderLocation::Type::UNIFORM, ShaderProgram::StandardLocation::PROJECTION_MATRIX_UNIFORM, "uPMatrix" );
                _program->registerStandardLocation( ShaderLocation::Type::UNIFORM, ShaderProgram::StandardLocation::VIEW_MATRIX_UNIFORM, "uVMatrix" );
                _program->registerStandardLocation( ShaderLocation::Type::UNIFORM, ShaderProgram::StandardLocation::MODEL_MATRIX_UNIFORM, "uMMatrix" );
			}
			
			virtual void execute( RenderGraph *graph, Renderer *renderer, RenderQueue *renderQueue ) override
			{
				CRIMILD_PROFILE( "Render Opaque Objects" )

                _gBuffer = graph->createFBO( { _depthOutput, _normalOutput } );

                renderer->bindFrameBuffer( crimild::get_ptr( _gBuffer ) );

				renderer->setColorMaskState( ColorMaskState::DISABLED );
				renderObjects( renderer, renderQueue, RenderQueue::RenderableType::OCCLUDER );
				renderer->setColorMaskState( ColorMaskState::ENABLED );
				
				renderObjects( renderer, renderQueue, RenderQueue::RenderableType::OPAQUE );
				
				renderer->unbindFrameBuffer( crimild::get_ptr( _gBuffer ) );
			}

			void renderObjects( Renderer *renderer, RenderQueue *renderQueue, RenderQueue::RenderableType renderableType )
			{
				auto renderables = renderQueue->getRenderables( renderableType );
				if ( renderables->size() == 0 ) {
					return;
				}

                auto program = crimild::get_ptr( _program );

				renderQueue->each( renderables, [this, renderer, renderQueue, program ]( RenderQueue::Renderable *renderable ) {
					auto material = crimild::get_ptr( renderable->material );

					renderer->bindProgram( program );
					
					auto projection = renderQueue->getProjectionMatrix();
					renderer->bindUniform( program->getStandardLocation( ShaderProgram::StandardLocation::PROJECTION_MATRIX_UNIFORM ), projection );
					
					auto view = renderQueue->getViewMatrix();
					renderer->bindUniform( program->getStandardLocation( ShaderProgram::StandardLocation::VIEW_MATRIX_UNIFORM ), view );
				
					renderStandardGeometry( renderer, crimild::get_ptr( renderable->geometry ), program, material, renderable->modelTransform );
					
					renderer->unbindProgram( program );
				});
			}

			// TODO: bind normal maps
			void renderStandardGeometry( Renderer *renderer, Geometry *geometry, ShaderProgram *program, Material *material, const Matrix4f &modelTransform )
			{
				renderer->bindUniform( program->getStandardLocation( ShaderProgram::StandardLocation::MODEL_MATRIX_UNIFORM ), modelTransform );
				
				auto rc = geometry->getComponent< RenderStateComponent >();
				renderer->bindUniform( program->getStandardLocation( ShaderProgram::StandardLocation::SKINNED_MESH_JOINT_COUNT_UNIFORM ), 0 );
				if ( auto skeleton = rc->getSkeleton() ) {
					renderer->bindUniform( program->getStandardLocation( ShaderProgram::StandardLocation::SKINNED_MESH_JOINT_COUNT_UNIFORM ), ( int ) skeleton->getJoints().size() );
					skeleton->getJoints().each( [ renderer, program ]( const std::string &, SharedPointer< animation::Joint > const &joint ) {
						renderer->bindUniform(
							program->getStandardLocation( ShaderProgram::StandardLocation::SKINNED_MESH_JOINT_POSE_UNIFORM + joint->getId() ),
							joint->getPoseMatrix()
							);
					});
				}
				
				geometry->forEachPrimitive( [renderer, program]( Primitive *primitive ) {
					// TODO: maybe we shound't add a geometry to the queue if it
					// has no valid primitive instead of quering the state of the
					// VBO and IBO while rendering
					
					auto vbo = primitive->getVertexBuffer();
					if ( vbo == nullptr ) {
						return;
					}
					
					auto ibo = primitive->getIndexBuffer();
					if ( ibo == nullptr ) {
						return;
					}
					
					renderer->bindVertexBuffer( program, vbo );
					renderer->bindIndexBuffer( program, ibo );
					
					renderer->drawPrimitive( program, primitive );
					
					renderer->unbindVertexBuffer( program, vbo );
					renderer->unbindIndexBuffer( program, ibo );
				});
			}
			
		private:
			SharedPointer< FrameBufferObject > _gBuffer;
			SharedPointer< ShaderProgram > _program;
			
			RenderGraphAttachment *_depthOutput = nullptr;
			RenderGraphAttachment *_normalOutput = nullptr;
		};



		class OpaquePass : public RenderGraphPass {
            CRIMILD_IMPLEMENT_RTTI( crimild::rendergraph::OpaquePass )
		public:
            OpaquePass( std::string name = "Render Opaque Objects" )
                : RenderGraphPass( name )
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
				if ( _depthInput == nullptr ) {
					_depthInput = graph->createAttachment(
                        "Aux Depth Buffer",
						RenderGraphAttachment::Hint::FORMAT_DEPTH |
						RenderGraphAttachment::Hint::RENDER_ONLY );
                    _clearFlags = FrameBufferObject::ClearFlag::ALL;
				}

                graph->read( this, { _depthInput } );
				graph->write( this, { _colorOutput } );

				_program = crimild::alloc< opengl::StandardShaderProgram >();
			}
			
			virtual void execute( RenderGraph *graph, Renderer *renderer, RenderQueue *renderQueue ) override
			{
                _gBuffer = graph->createFBO( { _depthInput, _colorOutput } );
                _gBuffer->setClearFlags( _clearFlags );

				CRIMILD_PROFILE( "Render Opaque Objects" )
				
				renderer->bindFrameBuffer( crimild::get_ptr( _gBuffer ) );
				
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
					
					renderStandardGeometry( renderer, crimild::get_ptr( renderable->geometry ), program, material, renderable->modelTransform );
					
					renderQueue->each( [ renderer, program ]( Light *light, int ) {
						renderer->unbindLight( program, light );
					});
					
					renderer->unbindProgram( program );
				});
				
				renderer->unbindFrameBuffer( crimild::get_ptr( _gBuffer ) );
			}
			
			void renderStandardGeometry( Renderer *renderer, Geometry *geometry, ShaderProgram *program, Material *material, const Matrix4f &modelTransform )
			{
				if ( material != nullptr ) {
					renderer->bindMaterial( program, material );
				}
				
				renderer->bindUniform( program->getStandardLocation( ShaderProgram::StandardLocation::MODEL_MATRIX_UNIFORM ), modelTransform );
				
				auto rc = geometry->getComponent< RenderStateComponent >();
				renderer->bindUniform( program->getStandardLocation( ShaderProgram::StandardLocation::SKINNED_MESH_JOINT_COUNT_UNIFORM ), 0 );
				if ( auto skeleton = rc->getSkeleton() ) {
					renderer->bindUniform( program->getStandardLocation( ShaderProgram::StandardLocation::SKINNED_MESH_JOINT_COUNT_UNIFORM ), ( int ) skeleton->getJoints().size() );
					skeleton->getJoints().each( [ renderer, program ]( const std::string &, SharedPointer< animation::Joint > const &joint ) {
						renderer->bindUniform(
							program->getStandardLocation( ShaderProgram::StandardLocation::SKINNED_MESH_JOINT_POSE_UNIFORM + joint->getId() ),
							joint->getPoseMatrix()
							);
					});
				}

				auto depthState = crimild::alloc< DepthState >( true, DepthState::CompareFunc::EQUAL, false );
				renderer->setDepthState( depthState );
				
				geometry->forEachPrimitive( [renderer, program]( Primitive *primitive ) {
					// TODO: maybe we shound't add a geometry to the queue if it
					// has no valid primitive instead of quering the state of the
					// VBO and IBO while rendering
					
					auto vbo = primitive->getVertexBuffer();
					if ( vbo == nullptr ) {
						return;
					}
					
					auto ibo = primitive->getIndexBuffer();
					if ( ibo == nullptr ) {
						return;
					}
					
					renderer->bindVertexBuffer( program, vbo );
					renderer->bindIndexBuffer( program, ibo );
					
					renderer->drawPrimitive( program, primitive );
					
					renderer->unbindVertexBuffer( program, vbo );
					renderer->unbindIndexBuffer( program, ibo );
				});
				
				if ( material != nullptr ) {
					renderer->unbindMaterial( program, material );
				}

				renderer->setDepthState( DepthState::ENABLED );
			}
			
		private:
			SharedPointer< ShaderProgram > _program;
			SharedPointer< FrameBufferObject > _gBuffer;
            crimild::Int8 _clearFlags;

			RenderGraphAttachment *_depthInput = nullptr;
			RenderGraphAttachment *_colorOutput = nullptr;
		};

		class TranslucentPass : public RenderGraphPass {
            CRIMILD_IMPLEMENT_RTTI( crimild::rendergraph::TranslucentPass )
		public:
            TranslucentPass( std::string name = "Render Translucent Objects" )
                : RenderGraphPass( name )
			{

			}
			
			virtual ~TranslucentPass( void )
			{
				
			}

			void setDepthInput( RenderGraphAttachment *attachment ) { _depthInput = attachment; }
			RenderGraphAttachment *getDepthInput( void ) { return _depthInput; }
			
			void setColorInput( RenderGraphAttachment *attachment ) { _colorInput = attachment; }
			RenderGraphAttachment *getColorInput( void ) { return _colorInput; }
			
			void setColorOutput( RenderGraphAttachment *attachment ) { _colorOutput = attachment; }
			RenderGraphAttachment *getColorOutput( void ) { return _colorOutput; }
			
			virtual void setup( rendergraph::RenderGraph *graph ) override
			{
                _clearFlags = FrameBufferObject::ClearFlag::COLOR;
                if ( _depthInput == nullptr ) {
                    _depthInput = graph->createAttachment(
                    "Aux Depth Buffer",
                    RenderGraphAttachment::Hint::FORMAT_DEPTH | RenderGraphAttachment::Hint::RENDER_ONLY );
                    _clearFlags = FrameBufferObject::ClearFlag::ALL;
                }

                graph->read( this, { _depthInput } );
				graph->write( this, { _colorOutput } );

				_program = crimild::alloc< opengl::StandardShaderProgram >();
			}
			
			virtual void execute( RenderGraph *graph, Renderer *renderer, RenderQueue *renderQueue ) override
			{
                _gBuffer = graph->createFBO( { _depthInput, _colorOutput } );
                _gBuffer->setClearFlags( _clearFlags );

				CRIMILD_PROFILE( "Render Opaque Objects" )
				
				renderer->bindFrameBuffer( crimild::get_ptr( _gBuffer ) );
				
				auto renderables = renderQueue->getRenderables( RenderQueue::RenderableType::TRANSLUCENT );
				if ( renderables->size() == 0 ) {
					return;
				}

				renderQueue->each( renderables, [this, renderer, renderQueue]( RenderQueue::Renderable *renderable ) {
					auto material = crimild::get_ptr( renderable->material );
					auto program = material->getProgram();
					if ( program == nullptr ) {
						program = crimild::get_ptr( _program );
					}
					
					renderer->bindProgram( program );
					
					auto projection = renderQueue->getProjectionMatrix();
					renderer->bindUniform( program->getStandardLocation( ShaderProgram::StandardLocation::PROJECTION_MATRIX_UNIFORM ), projection );
					
					auto view = renderQueue->getViewMatrix();
					renderer->bindUniform( program->getStandardLocation( ShaderProgram::StandardLocation::VIEW_MATRIX_UNIFORM ), view );
					
					renderQueue->each( [renderer, program]( Light *light, int ) {
						renderer->bindLight( program, light );
					});
					
					renderStandardGeometry( renderer, crimild::get_ptr( renderable->geometry ), program, material, renderable->modelTransform );
					
					renderQueue->each( [ renderer, program ]( Light *light, int ) {
						renderer->unbindLight( program, light );
					});
					
					renderer->unbindProgram( program );
				});
				
				renderer->unbindFrameBuffer( crimild::get_ptr( _gBuffer ) );
			}
			
			void renderStandardGeometry( Renderer *renderer, Geometry *geometry, ShaderProgram *program, Material *material, const Matrix4f &modelTransform )
			{
				if ( material != nullptr ) {
					renderer->bindMaterial( program, material );
				}
				
				renderer->bindUniform( program->getStandardLocation( ShaderProgram::StandardLocation::MODEL_MATRIX_UNIFORM ), modelTransform );
				
				auto rc = geometry->getComponent< RenderStateComponent >();
				renderer->bindUniform( program->getStandardLocation( ShaderProgram::StandardLocation::SKINNED_MESH_JOINT_COUNT_UNIFORM ), 0 );
				if ( auto skeleton = rc->getSkeleton() ) {
					renderer->bindUniform( program->getStandardLocation( ShaderProgram::StandardLocation::SKINNED_MESH_JOINT_COUNT_UNIFORM ), ( int ) skeleton->getJoints().size() );
					skeleton->getJoints().each( [ renderer, program ]( const std::string &, SharedPointer< animation::Joint > const &joint ) {
						renderer->bindUniform(
							program->getStandardLocation( ShaderProgram::StandardLocation::SKINNED_MESH_JOINT_POSE_UNIFORM + joint->getId() ),
							joint->getPoseMatrix()
							);
					});
				}

                auto depthState = crimild::alloc< DepthState >( true, DepthState::CompareFunc::LEQUAL, false );
                renderer->setDepthState( depthState );

				geometry->forEachPrimitive( [renderer, program]( Primitive *primitive ) {
					// TODO: maybe we shound't add a geometry to the queue if it
					// has no valid primitive instead of quering the state of the
					// VBO and IBO while rendering
					
					auto vbo = primitive->getVertexBuffer();
					if ( vbo == nullptr ) {
						return;
					}
					
					auto ibo = primitive->getIndexBuffer();
					if ( ibo == nullptr ) {
						return;
					}
					
					renderer->bindVertexBuffer( program, vbo );
					renderer->bindIndexBuffer( program, ibo );
					
					renderer->drawPrimitive( program, primitive );
					
					renderer->unbindVertexBuffer( program, vbo );
					renderer->unbindIndexBuffer( program, ibo );
				});
				
				if ( material != nullptr ) {
					renderer->unbindMaterial( program, material );
				}

                renderer->setDepthState( DepthState::ENABLED );
			}

		public:
			SharedPointer< ShaderProgram > _program;
			SharedPointer< FrameBufferObject > _gBuffer;
            crimild::Int8 _clearFlags;

			RenderGraphAttachment *_depthInput = nullptr;
			RenderGraphAttachment *_colorInput = nullptr;
			RenderGraphAttachment *_colorOutput = nullptr;			
		};

		class DepthToColorPass : public RenderGraphPass {
			CRIMILD_IMPLEMENT_RTTI( crimild::rendergraph::DepthToColorPass )
		public:
            DepthToColorPass( std::string name = "Convert Depth to RGB" ) : RenderGraphPass( name ) { }
			virtual ~DepthToColorPass( void ) { }

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
				
				auto program = renderer->getShaderProgram( Renderer::SHADER_PROGRAM_DEBUG_DEPTH );
				assert( program && "No valid program to render texture" );

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
			
			RenderGraphAttachment *_input = nullptr;
			RenderGraphAttachment *_output = nullptr;
		};

		
		class BlendPass : public RenderGraphPass {
			CRIMILD_IMPLEMENT_RTTI( crimild::rendergraph::BlendPass )
		public:
            BlendPass( std::string name = "Blend Inputs" ) : RenderGraphPass( name ) { }
			virtual ~BlendPass( void ) { }

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

				renderer->bindFrameBuffer( crimild::get_ptr( _gBuffer ) );

                renderer->setAlphaState( AlphaState::ENABLED_ADDITIVE_BLEND );
                renderer->setDepthState( DepthState::DISABLED );

				_inputs.each( [ this, renderer ]( RenderGraphAttachment *input ) {
					render( renderer, input->getTexture() );
				});

                renderer->setAlphaState( AlphaState::DISABLED );
                renderer->setDepthState( DepthState::ENABLED );
				
				renderer->unbindFrameBuffer( crimild::get_ptr( _gBuffer ) );
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

		class ColorTintPass : public RenderGraphPass {
			CRIMILD_IMPLEMENT_RTTI( crimild::rendergraph::ColorTintPass )

		public:
            ColorTintPass( std::string name = "Color Tint", const RGBAColorf &tint = RGBAColorf( 0.4392156863f, 0.2588235294f, 0.07843137255f, 1.0f ), crimild::Real32 value = 1.0f )
                : RenderGraphPass( name )
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
            FrameDebugPass( std::string name = "Debug Frame" ) : RenderGraphPass( name ) { }
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
                _gBuffer->setClearColor( RGBAColorf( 1.0f, 0.0f, 1.0f, 1.0f ) );

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
		
		class PresentPass : public RenderGraphPass {
			CRIMILD_IMPLEMENT_RTTI( crimild::rendergraph::PresentPass )
		public:
            PresentPass( std::string name = "Present Frame" )
                : RenderGraphPass( name )
			{
				
			}
			
			virtual ~PresentPass( void )
			{
				
			}
			
			void setInput( RenderGraphAttachment *attachment ) { _input = attachment; }
			RenderGraphAttachment *getInput( void ) { return _input; }
			
			virtual void setup( RenderGraph *graph ) override
			{
				graph->read( this, { _input } );
			}
			
			virtual void execute( RenderGraph *graph, Renderer *renderer, RenderQueue *renderQueue ) override
			{
				if ( _input == nullptr || _input->getTexture() == nullptr ) {
					return;
				}

				auto program = renderer->getShaderProgram( Renderer::SHADER_PROGRAM_SCREEN_TEXTURE );
				assert( program && "No valid program to render texture" );

				renderer->bindProgram( program );

				renderer->bindTexture(
					program->getStandardLocation( ShaderProgram::StandardLocation::COLOR_MAP_UNIFORM ),
					getInput()->getTexture() );

				renderer->drawScreenPrimitive( program );
				
				renderer->unbindTexture(
					program->getStandardLocation( ShaderProgram::StandardLocation::COLOR_MAP_UNIFORM ),
					getInput()->getTexture() );
				
				renderer->unbindProgram( program );
			}
			
		private:
			RenderGraphAttachment *_input = nullptr;
		};
		
		
		class RenderGraphRenderPassHelper : public RenderPass {
		public:
			RenderGraphRenderPassHelper( SharedPointer< rendergraph::RenderGraph > const &renderGraph )
				: _renderGraph( renderGraph )
			{
				
			}
			
			virtual ~RenderGraphRenderPassHelper( void )
			{
				
			}

			virtual void render( Renderer *renderer, RenderQueue *renderQueue, Camera *camera ) override
			{
				_renderGraph->execute( renderer, renderQueue );
			}
			
		private:
			SharedPointer< rendergraph::RenderGraph > _renderGraph;
		};
		
	}

}

using namespace crimild;
using namespace crimild::rendergraph;

SharedPointer< RenderGraph > createRenderGraph( crimild::Bool useDebugPass )
{
	auto renderGraph = crimild::alloc< RenderGraph >();

    auto depthPass = renderGraph->createPass< DepthPass >();
	auto gPass = renderGraph->createPass< OpaquePass >();
    auto translucentPass = renderGraph->createPass< TranslucentPass >();
    auto blendTranslucentPass = renderGraph->createPass< BlendPass >();
    auto sepiaPass = renderGraph->createPass< ColorTintPass >();
	auto presentPass = renderGraph->createPass< PresentPass >();

    auto depthBuffer = renderGraph->createAttachment( "Depth", RenderGraphAttachment::Hint::FORMAT_DEPTH_HDR );
    auto colorBuffer = renderGraph->createAttachment( "Color", RenderGraphAttachment::Hint::FORMAT_RGBA_HDR );
    auto translucentBuffer = renderGraph->createAttachment( "Translucent", RenderGraphAttachment::Hint::FORMAT_RGBA_HDR );
    auto blendedTranslucentBuffer = renderGraph->createAttachment( "Color + Translucent", RenderGraphAttachment::Hint::FORMAT_RGBA_HDR );
    auto sepiaBuffer = renderGraph->createAttachment( "Sepia", RenderGraphAttachment::Hint::FORMAT_RGBA );

    depthPass->setDepthOutput( depthBuffer );

    gPass->setDepthInput( depthBuffer );
	gPass->setColorOutput( colorBuffer );

    translucentPass->setDepthInput( depthBuffer );
    translucentPass->setColorOutput( translucentBuffer );

    blendTranslucentPass->addInput( colorBuffer );
    blendTranslucentPass->addInput( translucentBuffer );
    blendTranslucentPass->setOutput( blendedTranslucentBuffer );

    sepiaPass->setInput( blendedTranslucentBuffer );
    sepiaPass->setOutput( sepiaBuffer );

    if ( useDebugPass ) {
        auto normalBuffer = renderGraph->createAttachment( "Normal", RenderGraphAttachment::Hint::FORMAT_RGBA_HDR );
        depthPass->setNormalOutput( normalBuffer );

        auto depthToColorPass = renderGraph->createPass< DepthToColorPass >();
        auto depthColorBuffer = renderGraph->createAttachment( "Depth RGB", RenderGraphAttachment::Hint::FORMAT_RGBA );
        depthToColorPass->setInput( depthBuffer );
        depthToColorPass->setOutput( depthColorBuffer );

        auto debugPass = renderGraph->createPass< FrameDebugPass >();
        auto debugBuffer = renderGraph->createAttachment( "Debug", RenderGraphAttachment::Hint::FORMAT_RGBA );
        debugPass->addInput( sepiaBuffer );
        debugPass->addInput( depthColorBuffer );
        debugPass->addInput( normalBuffer );
        debugPass->addInput( colorBuffer );
        debugPass->addInput( translucentBuffer );
        debugPass->addInput( blendedTranslucentBuffer );
        debugPass->setOutput( debugBuffer );

        presentPass->setInput( debugBuffer );
    }
    else {
        presentPass->setInput( sepiaBuffer );
    }

	return renderGraph;
}

SharedPointer< Node > createTeapots( void )
{
	auto scene = crimild::alloc< Group >();

	auto teapot = []( const Vector3f &position, const RGBAColorf &color, crimild::Bool isOccluder = false ) -> SharedPointer< Node > {
		auto geometry = crimild::alloc< Geometry >();
		auto primitive = crimild::alloc< NewellTeapotPrimitive >();
		geometry->attachPrimitive( primitive );
		geometry->perform( UpdateWorldState() );
		geometry->local().setScale( 10.0f / geometry->getWorldBound()->getRadius() );
		geometry->local().setTranslate( position );

		auto m = crimild::alloc< Material >();
		m->setDiffuse( color );
		if ( color.a() < 1.0f ) m->setAlphaState( AlphaState::ENABLED );
		if ( isOccluder ) m->setColorMaskState( ColorMaskState::DISABLED );
        geometry->getComponent< MaterialComponent >()->attachMaterial( m );

		return geometry;
	};

	scene->attachNode( teapot( Vector3f( -10.0f, 0.0f, 10.0f ), RGBAColorf( 1.0f, 1.0f, 1.0f, 1.0f ) ) );
	scene->attachNode( teapot( Vector3f( 10.0f, 0.0f, 10.0f ), RGBAColorf( 1.0f, 1.0f, 1.0f, 1.0f ) ) );
	scene->attachNode( teapot( Vector3f( -10.0f, 0.0f, -10.0f ), RGBAColorf( 1.0f, 1.0f, 1.0f, 1.0f ) ) );
	scene->attachNode( teapot( Vector3f( 10.0f, 0.0f, -10.0f ), RGBAColorf( 1.0f, 1.0f, 1.0f, 0.75f ) ) );
    scene->attachNode( teapot( Vector3f( 0.0f, 0.0f, 0.0f ), RGBAColorf( 1.0f, 1.0f, 1.0f, 1.0f ), true ) );

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

int main( int argc, char **argv )
{
    auto settings = crimild::alloc< Settings >( argc, argv );
    settings->set( "video.width", 1280 );
    settings->set( "video.height", 900 );
    settings->set( "video.show_frame_time", true );
    auto sim = crimild::alloc< sdl::SDLSimulation >( "Render Graphs", settings );

    auto scene = crimild::alloc< Group >();

	auto teapots = createTeapots();
	teapots->attachComponent< RotationComponent >( Vector3f::UNIT_Y, 0.05f );
	scene->attachNode( teapots );

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
    camera->setRenderPass( crimild::alloc< RenderGraphRenderPassHelper >( renderGraph ) );
	scene->attachNode( camera );
    
    sim->setScene( scene );

	sim->registerMessageHandler< crimild::messaging::KeyReleased >( [ camera ]( crimild::messaging::KeyReleased const &msg ) {
		switch ( msg.key ) {
			case CRIMILD_INPUT_KEY_Q:
				crimild::concurrency::sync_frame( [ camera ]() {
					std::cout << "RenderGraph" << std::endl;
					Renderer::getInstance()->setFrameBuffer( RenderPass::S_BUFFER_NAME, nullptr );
					auto renderGraph = createRenderGraph( false );
					camera->setRenderPass( crimild::alloc< RenderGraphRenderPassHelper >( renderGraph ) );
				});
				break;

            case CRIMILD_INPUT_KEY_W:
                crimild::concurrency::sync_frame( [ camera ]() {
                    std::cout << "RenderGraph (Debug)" << std::endl;
                    Renderer::getInstance()->setFrameBuffer( RenderPass::S_BUFFER_NAME, nullptr );
                    auto renderGraph = createRenderGraph( true );
                    camera->setRenderPass( crimild::alloc< RenderGraphRenderPassHelper >( renderGraph ) );
                });
                break;

			case CRIMILD_INPUT_KEY_E:
				crimild::concurrency::sync_frame( [ camera ]() {
					std::cout << "RenderPass (post)" << std::endl;
					Renderer::getInstance()->setFrameBuffer( RenderPass::S_BUFFER_NAME, nullptr );
					camera->setRenderPass( crimild::alloc< PostRenderPass >( crimild::alloc< StandardRenderPass >() ) );
					auto sepiaToneEffect = crimild::alloc< ColorTintImageEffect >( ColorTintImageEffect::TINT_SEPIA );
					camera->getRenderPass()->getImageEffects().add( sepiaToneEffect );
				});
				break;

			case CRIMILD_INPUT_KEY_R:
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

