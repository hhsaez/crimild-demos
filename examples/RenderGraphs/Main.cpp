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
#include "Rendering/RenderGraph/RenderGraphResource.hpp"

namespace crimild {

	namespace rendergraph {

		class DepthPass : public RenderGraphPass {
		public:
			DepthPass( void )
			{

			}
			
			virtual ~DepthPass( void )
			{
				
			}
			
			void setDepthOutput( RenderGraphResource *resource ) { _depthOutput = resource; }
			RenderGraphResource *getDepthOutput( void ) { return _depthOutput; }
			
			void setNormalOutput( RenderGraphResource *resource ) { _normalOutput = resource; }
			RenderGraphResource *getNormalOutput( void ) { return _normalOutput; }
			
			virtual void setup( rendergraph::RenderGraph *graph )
			{
				auto renderer = Renderer::getInstance();
				
				int width = renderer->getScreenBuffer()->getWidth();
				int height = renderer->getScreenBuffer()->getHeight();
				
				_gBuffer = crimild::alloc< FrameBufferObject >( width, height );
				
				if ( _depthOutput != nullptr ) {
					_depthRT = crimild::alloc< RenderTarget >( RenderTarget::Type::DEPTH_24, RenderTarget::Output::RENDER_AND_TEXTURE, width, height, true );
					_depthOutput->setTexture( _depthRT->getTexture() );
					_depthOutput->setRenderTarget( crimild::get_ptr( _depthRT ) );
				}
				else {
					_depthRT = crimild::alloc< RenderTarget >( RenderTarget::Type::DEPTH_24, RenderTarget::Output::RENDER, width, height );
				}
				_gBuffer->getRenderTargets().insert( RenderPass::S_BUFFER_DEPTH_TARGET_NAME, _depthRT );
				
				_colorRT = crimild::alloc< RenderTarget >( RenderTarget::Type::COLOR_RGBA, RenderTarget::Output::RENDER_AND_TEXTURE, width, height );
				if ( _normalOutput != nullptr ) {
					_normalOutput->setTexture( _colorRT->getTexture() );
				}
				_gBuffer->getRenderTargets().insert( RenderPass::S_BUFFER_COLOR_TARGET_NAME, _colorRT );
				
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
			
			virtual void execute( Renderer *renderer, RenderQueue *renderQueue )
			{
				CRIMILD_PROFILE( "Render Opaque Objects" )
				
				renderer->bindFrameBuffer( crimild::get_ptr( _gBuffer ) );
				
				auto renderables = renderQueue->getRenderables( RenderQueue::RenderableType::OPAQUE );
				if ( renderables->size() == 0 ) {
					return;
				}

//                auto program = AssetManager::getInstance()->get< ShaderProgram >( Renderer::SHADER_PROGRAM_RENDER_PASS_STANDARD );
                auto program = crimild::get_ptr( _program );

				renderQueue->each( renderables, [this, renderer, renderQueue, program ]( RenderQueue::Renderable *renderable ) {
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
			}
			
		private:
			SharedPointer< FrameBufferObject > _gBuffer;
			SharedPointer< RenderTarget > _depthRT;
			SharedPointer< RenderTarget > _colorRT;
			SharedPointer< ShaderProgram > _program;
			
			RenderGraphResource *_depthOutput = nullptr;
			RenderGraphResource *_normalOutput = nullptr;
		};



		class OpaquePass : public RenderGraphPass {
		public:
			OpaquePass( void )
			{

			}
			
			virtual ~OpaquePass( void )
			{
				
			}
			
			void setDepthInput( RenderGraphResource *resource ) { _depthInput = resource; }
			RenderGraphResource *getDepthInput( void ) { return _depthInput; }
			
			void setColorOutput( RenderGraphResource *resource ) { _colorOutput = resource; }
			RenderGraphResource *getColorOutput( void ) { return _colorOutput; }
			
			virtual void setup( rendergraph::RenderGraph *graph )
			{
				auto renderer = Renderer::getInstance();
				
				int width = renderer->getScreenBuffer()->getWidth();
				int height = renderer->getScreenBuffer()->getHeight();
				
				_gBuffer = crimild::alloc< FrameBufferObject >( width, height );
				
				if ( _depthInput != nullptr ) {
                    _depthRT = crimild::retain( _depthInput->getRenderTarget() );
				}
				else {
					_depthRT = crimild::alloc< RenderTarget >( RenderTarget::Type::DEPTH_24, RenderTarget::Output::RENDER, width, height );
				}
				_gBuffer->getRenderTargets().insert( RenderPass::S_BUFFER_DEPTH_TARGET_NAME, _depthRT );
				/*
				if ( _depthOutput != nullptr ) {
					_depthRT = crimild::alloc< RenderTarget >( RenderTarget::Type::DEPTH_24, RenderTarget::Output::RENDER_AND_TEXTURE, width, height, true );
					_depthOutput->setTexture( _depthRT->getTexture() );
					_depthOutput->setRenderTarget( crimild::get_ptr( _depthRT ) );
					//_depthOutput->setFormat( RenderPassResource::Format::R32G0B0A0 );
				}
				else {
					_depthRT = crimild::alloc< RenderTarget >( RenderTarget::Type::DEPTH_24, RenderTarget::Output::RENDER, width, height );
				}
				_gBuffer->getRenderTargets().insert( RenderPass::S_BUFFER_DEPTH_TARGET_NAME, _depthRT );
				*/
				
				_colorRT = crimild::alloc< RenderTarget >( RenderTarget::Type::COLOR_RGBA, RenderTarget::Output::RENDER_AND_TEXTURE, width, height );
				if ( _colorOutput != nullptr ) {
					_colorOutput->setTexture( _colorRT->getTexture() );
				}
				_gBuffer->getRenderTargets().insert( RenderPass::S_BUFFER_COLOR_TARGET_NAME, _colorRT );

				_gBuffer->setClearFlags( FrameBufferObject::ClearFlag::COLOR );

                graph->read( this, { _depthInput } );
				graph->write( this, { _colorOutput } );
			}
			
			virtual void execute( Renderer *renderer, RenderQueue *renderQueue )
			{
				if ( _gBuffer == nullptr ) {
				}
				
				CRIMILD_PROFILE( "Render Opaque Objects" )
				
				renderer->bindFrameBuffer( crimild::get_ptr( _gBuffer ) );
				
				auto renderables = renderQueue->getRenderables( RenderQueue::RenderableType::OPAQUE );
				if ( renderables->size() == 0 ) {
					return;
				}

				auto program = AssetManager::getInstance()->get< ShaderProgram >( Renderer::SHADER_PROGRAM_RENDER_PASS_STANDARD );

				
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
			
		private:
			SharedPointer< FrameBufferObject > _gBuffer;
			SharedPointer< RenderTarget > _depthRT;
			SharedPointer< RenderTarget > _colorRT;
			
			RenderGraphResource *_depthInput = nullptr;
			RenderGraphResource *_colorOutput = nullptr;
		};

		class TranslucentPass : public RenderGraphPass {
		public:
			TranslucentPass( void )
			{

			}
			
			virtual ~TranslucentPass( void )
			{
				
			}

			void setDepthInput( RenderGraphResource *resource ) { _depthInput = resource; }
			RenderGraphResource *getDepthInput( void ) { return _depthInput; }
			
			void setColorInput( RenderGraphResource *resource ) { _colorInput = resource; }
			RenderGraphResource *getColorInput( void ) { return _colorInput; }
			
			void setColorOutput( RenderGraphResource *resource ) { _colorOutput = resource; }
			RenderGraphResource *getColorOutput( void ) { return _colorOutput; }
			
			virtual void setup( rendergraph::RenderGraph *graph )
			{
				auto renderer = Renderer::getInstance();
				
				int width = renderer->getScreenBuffer()->getWidth();
				int height = renderer->getScreenBuffer()->getHeight();
				
				_gBuffer = crimild::alloc< FrameBufferObject >( width, height );

				if ( _depthInput != nullptr ) {
                    _depthRT = crimild::retain( _depthInput->getRenderTarget() );
				}
				else {
					_depthRT = crimild::alloc< RenderTarget >( RenderTarget::Type::DEPTH_24, RenderTarget::Output::RENDER, width, height );
				}
				_gBuffer->getRenderTargets().insert( RenderPass::S_BUFFER_DEPTH_TARGET_NAME, _depthRT );

				_colorRT = crimild::alloc< RenderTarget >( RenderTarget::Type::COLOR_RGBA, RenderTarget::Output::RENDER_AND_TEXTURE, width, height );
				
				if ( _colorOutput != nullptr ) {
					_colorOutput->setTexture( _colorRT->getTexture() );
				}
				_gBuffer->getRenderTargets().insert( RenderPass::S_BUFFER_COLOR_TARGET_NAME, _colorRT );

				_gBuffer->setClearFlags( FrameBufferObject::ClearFlag::COLOR );

                graph->read( this, { _depthInput } );
				graph->write( this, { _colorOutput } );
			}
			
			virtual void execute( Renderer *renderer, RenderQueue *renderQueue )
			{
				if ( _gBuffer == nullptr ) {
				}
				
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
						program = AssetManager::getInstance()->get< ShaderProgram >( Renderer::SHADER_PROGRAM_RENDER_PASS_STANDARD );
					}
					
					renderer->bindProgram( program );
					
					auto projection = renderQueue->getProjectionMatrix();
					renderer->bindUniform( program->getStandardLocation( ShaderProgram::StandardLocation::PROJECTION_MATRIX_UNIFORM ), projection );
					
					auto view = renderQueue->getViewMatrix();
					renderer->bindUniform( program->getStandardLocation( ShaderProgram::StandardLocation::VIEW_MATRIX_UNIFORM ), view );
					
					renderStandardGeometry( renderer, crimild::get_ptr( renderable->geometry ), program, material, renderable->modelTransform );
					
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

//                renderer->setAlphaState( AlphaState::ENABLED );


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

//                renderer->setAlphaState( AlphaState::DISABLED );
                renderer->setDepthState( DepthState::ENABLED );
			}

		public:
			SharedPointer< FrameBufferObject > _gBuffer;
			SharedPointer< RenderTarget > _depthRT;
			SharedPointer< RenderTarget > _colorRT;
			
			RenderGraphResource *_depthInput = nullptr;
			RenderGraphResource *_colorInput = nullptr;
			RenderGraphResource *_colorOutput = nullptr;			
		};

		class DepthToColorPass : public RenderGraphPass {
			CRIMILD_IMPLEMENT_RTTI( crimild::rendergraph::DepthToColorPass )
		public:
			DepthToColorPass( void ) { }
			virtual ~DepthToColorPass( void ) { }

			void setInput( RenderGraphResource *resource ) { _input = resource; }
			RenderGraphResource *getInput( void ) { return _input; }

			void setOutput( RenderGraphResource *resource ) { _output = resource; }
			RenderGraphResource *getOutput( void ) { return _output; }

			virtual void setup( RenderGraph *graph ) override
			{
				graph->read( this, { _input } );
				graph->write( this, { _output } );

				auto renderer = Renderer::getInstance();
				
				int width = renderer->getScreenBuffer()->getWidth();
				int height = renderer->getScreenBuffer()->getHeight();
				
				_gBuffer = crimild::alloc< FrameBufferObject >( width, height );
				
				auto depthRT = crimild::alloc< RenderTarget >( RenderTarget::Type::DEPTH_24, RenderTarget::Output::RENDER, width, height );
				_gBuffer->getRenderTargets().insert( RenderPass::S_BUFFER_DEPTH_TARGET_NAME, depthRT );
				
				auto colorRT = crimild::alloc< RenderTarget >( RenderTarget::Type::COLOR_RGBA, RenderTarget::Output::RENDER_AND_TEXTURE, width, height );
				if ( _output != nullptr ) {
					_output->setTexture( colorRT->getTexture() );
				}
				_gBuffer->getRenderTargets().insert( RenderPass::S_BUFFER_COLOR_TARGET_NAME, colorRT );
			}

			virtual void execute( Renderer *renderer, RenderQueue *renderQueue ) override
			{
				if ( _input == nullptr || _input->getTexture() == nullptr ) {
					return;
				}

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
			
			RenderGraphResource *_input = nullptr;
			RenderGraphResource *_output = nullptr;
		};

		
		class BlendPass : public RenderGraphPass {
			CRIMILD_IMPLEMENT_RTTI( crimild::rendergraph::BlendPass )
		public:
			BlendPass( void ) { }
			virtual ~BlendPass( void ) { }

			void addInput( RenderGraphResource *input )
			{
				_inputs.add( input );
			}

			void setOutput( RenderGraphResource *resource ) { _output = resource; }
			RenderGraphResource *getOutput( void ) { return _output; }

			virtual void setup( RenderGraph *graph ) override
			{
				graph->read( this, _inputs );
				graph->write( this, { _output } );

				auto renderer = Renderer::getInstance();
				
				int width = renderer->getScreenBuffer()->getWidth();
				int height = renderer->getScreenBuffer()->getHeight();
				
				_gBuffer = crimild::alloc< FrameBufferObject >( width, height );
				
				auto depthRT = crimild::alloc< RenderTarget >( RenderTarget::Type::DEPTH_24, RenderTarget::Output::RENDER, width, height );
				_gBuffer->getRenderTargets().insert( RenderPass::S_BUFFER_DEPTH_TARGET_NAME, depthRT );
				
				auto colorRT = crimild::alloc< RenderTarget >( RenderTarget::Type::COLOR_RGBA, RenderTarget::Output::RENDER_AND_TEXTURE, width, height );
				if ( _output != nullptr ) {
					_output->setTexture( colorRT->getTexture() );
				}
				_gBuffer->getRenderTargets().insert( RenderPass::S_BUFFER_COLOR_TARGET_NAME, colorRT );
			}

			virtual void execute( Renderer *renderer, RenderQueue *renderQueue ) override
			{
				if ( _inputs.size() == 0 ) {
					return;
				}

				renderer->bindFrameBuffer( crimild::get_ptr( _gBuffer ) );

                renderer->setAlphaState( AlphaState::ENABLED_ADDITIVE_BLEND );
                renderer->setDepthState( DepthState::DISABLED );

				_inputs.each( [ this, renderer ]( RenderGraphResource *input ) {
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
			
			containers::Array< RenderGraphResource * > _inputs;
			
			RenderGraphResource *_output = nullptr;
		};

		class ColorTintPass : public RenderGraphPass {
			CRIMILD_IMPLEMENT_RTTI( crimild::rendergraph::ColorTintPass )

		public:
			ColorTintPass( const RGBAColorf &tint = RGBAColorf( 0.4392156863f, 0.2588235294f, 0.07843137255f, 1.0f ), crimild::Real32 value = 1.0f )
			{
				_program = crimild::alloc< crimild::opengl::ColorTintShaderProgram >();
				_program->attachUniform( crimild::alloc< RGBAColorfUniform >( ColorTintImageEffect::COLOR_TINT_UNIFORM_TINT, tint ) );
				_program->attachUniform( crimild::alloc< FloatUniform >( ColorTintImageEffect::COLOR_TINT_UNIFORM_TINT_VALUE, value ) );
			}

			virtual ~ColorTintPass( void )
			{

			}

			void setInput( RenderGraphResource *resource ) { _input = resource; }
			RenderGraphResource *getInput( void ) { return _input; }

			void setOutput( RenderGraphResource *resource ) { _output = resource; }
			RenderGraphResource *getOutput( void ) { return _output; }

			virtual void setup( RenderGraph *graph ) override
			{
				graph->read( this, { _input } );
				graph->write( this, { _output } );

				auto renderer = Renderer::getInstance();
				
				int width = renderer->getScreenBuffer()->getWidth();
				int height = renderer->getScreenBuffer()->getHeight();
				
				_gBuffer = crimild::alloc< FrameBufferObject >( width, height );
				
				auto depthRT = crimild::alloc< RenderTarget >( RenderTarget::Type::DEPTH_24, RenderTarget::Output::RENDER, width, height );
				_gBuffer->getRenderTargets().insert( RenderPass::S_BUFFER_DEPTH_TARGET_NAME, depthRT );
				
				auto colorRT = crimild::alloc< RenderTarget >( RenderTarget::Type::COLOR_RGBA, RenderTarget::Output::RENDER_AND_TEXTURE, width, height );
				if ( _output != nullptr ) {
					_output->setTexture( colorRT->getTexture() );
				}
				_gBuffer->getRenderTargets().insert( RenderPass::S_BUFFER_COLOR_TARGET_NAME, colorRT );

			}

			virtual void execute( Renderer *renderer, RenderQueue *renderQueue ) override
			{
				if ( _input == nullptr || _input->getTexture() == nullptr ) {
					return;
				}

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
			
			RenderGraphResource *_input = nullptr;
			RenderGraphResource *_output = nullptr;
		};
		
		class FrameDebugPass : public RenderGraphPass {
			CRIMILD_IMPLEMENT_RTTI( crimild::rendergraph::FrameDebugPass )

		public:
			FrameDebugPass( void ) { }
			virtual ~FrameDebugPass( void ) { }

			void addInput( RenderGraphResource *input )
			{
				_inputs.add( input );
			}

			void setOutput( RenderGraphResource *resource ) { _output = resource; }
			RenderGraphResource *getOutput( void ) { return _output; }

			virtual void setup( RenderGraph *graph ) override
			{
				graph->read( this, _inputs );
				graph->write( this, { _output } );

				auto renderer = Renderer::getInstance();
				
				int width = renderer->getScreenBuffer()->getWidth();
				int height = renderer->getScreenBuffer()->getHeight();
				
				_gBuffer = crimild::alloc< FrameBufferObject >( width, height );
				_gBuffer->setClearColor( RGBAColorf( 1.0f, 0.0f, 1.0f, 1.0f ) );
				
				auto depthRT = crimild::alloc< RenderTarget >( RenderTarget::Type::DEPTH_24, RenderTarget::Output::RENDER, width, height );
				_gBuffer->getRenderTargets().insert( RenderPass::S_BUFFER_DEPTH_TARGET_NAME, depthRT );
				
				auto colorRT = crimild::alloc< RenderTarget >( RenderTarget::Type::COLOR_RGBA, RenderTarget::Output::RENDER_AND_TEXTURE, width, height );
				if ( _output != nullptr ) {
					_output->setTexture( colorRT->getTexture() );
				}
				_gBuffer->getRenderTargets().insert( RenderPass::S_BUFFER_COLOR_TARGET_NAME, colorRT );
			}

			virtual void execute( Renderer *renderer, RenderQueue *renderQueue ) override
			{
				if ( _inputs.size() == 0 ) {
					return;
				}

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

				_inputs.each( [ this, renderer, &layout ]( RenderGraphResource *input, crimild::Size idx ) {
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

			containers::Array< RenderGraphResource * > _inputs;
			
			RenderGraphResource *_output = nullptr;
		};
		
		class PresentPass : public RenderGraphPass {
			CRIMILD_IMPLEMENT_RTTI( crimild::rendergraph::PresentPass )
		public:
			PresentPass( void )
			{
				
			}
			
			virtual ~PresentPass( void )
			{
				
			}
			
			void setInput( RenderGraphResource *resource ) { _input = resource; }
			RenderGraphResource *getInput( void ) { return _input; }
			
			virtual void setup( RenderGraph *graph ) override
			{
				graph->read( this, { _input } );
			}
			
			virtual void execute( Renderer *renderer, RenderQueue *renderQueue ) override
			{
				if ( _input == nullptr || _input->getTexture() == nullptr ) {
					return;
				}

				/*
				auto fbo = renderer->getFrameBuffer( RenderPass::S_BUFFER_NAME );
				if ( fbo == nullptr ) {
					int width = renderer->getScreenBuffer()->getWidth();
					int height = renderer->getScreenBuffer()->getHeight();
					
					Log::debug( CRIMILD_CURRENT_CLASS_NAME, "Creating S_BUFFER with size ", width, "x", height );
					
					auto sBuffer = crimild::alloc< FrameBufferObject >( width, height );
					sBuffer->getRenderTargets().insert( RenderPass::S_BUFFER_DEPTH_TARGET_NAME, crimild::alloc< RenderTarget >( RenderTarget::Type::DEPTH_24, RenderTarget::Output::RENDER, width, height ) );
					sBuffer->getRenderTargets().insert( RenderPass::S_BUFFER_COLOR_TARGET_NAME, crimild::alloc< RenderTarget >( RenderTarget::Type::COLOR_RGBA, RenderTarget::Output::RENDER_AND_TEXTURE, width, height ) );
					renderer->setFrameBuffer( RenderPass::S_BUFFER_NAME, sBuffer );
					fbo = crimild::get_ptr( sBuffer );
				}

				renderer->bindFrameBuffer( fbo );
				*/
				
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

				//renderer->unbindFrameBuffer( fbo );
			}
			
		private:
			RenderGraphResource *_input = nullptr;
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

SharedPointer< RenderGraph > createRenderGraph( void )
{
	auto renderGraph = crimild::alloc< RenderGraph >();

    auto depthPass = renderGraph->createPass< DepthPass >();
	auto gPass = renderGraph->createPass< OpaquePass >();
	auto translucentPass = renderGraph->createPass< TranslucentPass >();
	auto blendTranslucentPass = renderGraph->createPass< BlendPass >();
	auto sepiaPass = renderGraph->createPass< ColorTintPass >();
	auto depthToColorPass = renderGraph->createPass< DepthToColorPass >();
	auto debugPass = renderGraph->createPass< FrameDebugPass >();
	auto presentPass = renderGraph->createPass< PresentPass >();

    auto depthBuffer = renderGraph->createResource();
    auto normalBuffer = renderGraph->createResource();
	auto colorBuffer = renderGraph->createResource();
	auto translucentBuffer = renderGraph->createResource();
	auto blendedTranslucentBuffer = renderGraph->createResource();
	auto depthColorBuffer = renderGraph->createResource();
	auto sepiaBuffer = renderGraph->createResource();
	auto debugBuffer = renderGraph->createResource();

    depthPass->setDepthOutput( depthBuffer );
    depthPass->setNormalOutput( normalBuffer );

	gPass->setDepthInput( depthBuffer );
	gPass->setColorOutput( colorBuffer );

	translucentPass->setDepthInput( depthBuffer );
	translucentPass->setColorOutput( translucentBuffer );

	blendTranslucentPass->addInput( colorBuffer );
	blendTranslucentPass->addInput( translucentBuffer );
	blendTranslucentPass->setOutput( blendedTranslucentBuffer );

	depthToColorPass->setInput( depthBuffer );
	depthToColorPass->setOutput( depthColorBuffer );

	sepiaPass->setInput( blendedTranslucentBuffer );
	sepiaPass->setOutput( sepiaBuffer );

	debugPass->addInput( sepiaBuffer );
    debugPass->addInput( depthColorBuffer );
    debugPass->addInput( normalBuffer );
	debugPass->addInput( colorBuffer );
	debugPass->addInput( translucentBuffer );
	debugPass->addInput( blendedTranslucentBuffer );
	debugPass->setOutput( debugBuffer );

	presentPass->setInput( debugBuffer );
	
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

int main( int argc, char **argv )
{
    auto settings = crimild::alloc< Settings >( argc, argv );
    settings->set( "video.width", 1280 );
    settings->set( "video.height", 900 );
    auto sim = crimild::alloc< sdl::SDLSimulation >( "Render Graphs", settings );

    auto scene = crimild::alloc< Group >();

	auto teapots = createTeapots();
	teapots->attachComponent< RotationComponent >( Vector3f::UNIT_Y, 0.25f );
	scene->attachNode( teapots );

	auto light1 = crimild::alloc< Light >();
	light1->local().setTranslate( -50.0f, 15.0f, 30.0f );
	light1->setColor( RGBAColorf( 1.0f, 0.0f, 0.0f, 1.0f ) );
	scene->attachNode( light1 );

	auto light2 = crimild::alloc< Light >();
	light2->local().setTranslate( 50.0f, 15.0f, 0.0f );
	light2->setColor( RGBAColorf( 0.0f, 1.0f, 0.0f, 1.0f ) );
	scene->attachNode( light2 );

	auto light3 = crimild::alloc< Light >();
	light3->local().setTranslate( 0.0f, 50.0f, -50.0f );
	light3->setColor( RGBAColorf( 0.0f, 0.0f, 1.0f, 1.0f ) );
	scene->attachNode( light3 );

	auto camera = crimild::alloc< Camera >( 45.0f, 4.0f / 3.0f, 0.1f, 1024.0f );
	camera->local().setTranslate( 0.0f, 5.0f, 30.0f );
	camera->local().lookAt( Vector3f( 0.0f, 3.0f, 0.0f ) );
	auto renderGraph = createRenderGraph();
	camera->setRenderPass( crimild::alloc< RenderGraphRenderPassHelper >( renderGraph ) );
	scene->attachNode( camera );
    
    sim->setScene( scene );

	sim->registerMessageHandler< crimild::messaging::KeyReleased >( [ camera ]( crimild::messaging::KeyReleased const &msg ) {
		switch ( msg.key ) {
			case CRIMILD_INPUT_KEY_Q:
				crimild::concurrency::sync_frame( [ camera ]() {
					std::cout << "RenderGraph" << std::endl;
					Renderer::getInstance()->setFrameBuffer( RenderPass::S_BUFFER_NAME, nullptr );
					auto renderGraph = createRenderGraph();
					camera->setRenderPass( crimild::alloc< RenderGraphRenderPassHelper >( renderGraph ) );
				});
				break;

			case CRIMILD_INPUT_KEY_W:
				crimild::concurrency::sync_frame( [ camera ]() {
					std::cout << "RenderPass" << std::endl;
					Renderer::getInstance()->setFrameBuffer( RenderPass::S_BUFFER_NAME, nullptr );
					camera->setRenderPass( crimild::alloc< PostRenderPass >( crimild::alloc< StandardRenderPass >() ) );
					auto sepiaToneEffect = crimild::alloc< ColorTintImageEffect >( ColorTintImageEffect::TINT_SEPIA );
					camera->getRenderPass()->getImageEffects().add( sepiaToneEffect );
				});
				break;

			case CRIMILD_INPUT_KEY_E:
				crimild::concurrency::sync_frame( [ camera ]() {
					std::cout << "Default" << std::endl;
					Renderer::getInstance()->setFrameBuffer( RenderPass::S_BUFFER_NAME, nullptr );
					camera->setRenderPass( crimild::alloc< StandardRenderPass >() );
				});
				break;
		}
	});
	
	return sim->run();
}

