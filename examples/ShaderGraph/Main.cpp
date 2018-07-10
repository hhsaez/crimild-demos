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
#include <Crimild_OpenGL.hpp>
#include <Crimild_GLFW.hpp>

#include "Foundation/Containers/List.hpp"

namespace crimild {

	class StandardShaderInputs {
	public:
		static const char *VERTEX_POSITION;
		static const char *VERTEX_NORMAL;
		static const char *VERTEX_COLOR;
		static const char *VERTEX_TEXTURE_COORDS;
	};

	class StandardShaderUniforms {
	public:
		static const char *PROJECTION_MATRIX;
		static const char *VIEW_MATRIX;
		static const char *MODEL_MATRIX;
	};

	class StandardShaderOutputs {
	public:
		static const char *VERTEX_SCREEN_POSITION;
		static const char *VERTEX_COLOR;
		static const char *VERTEX_WORLD_POSITION;
	};

	class ShaderNode;
		
	class ShaderOutlet :
		public coding::Codable,
		public NamedObject {
		CRIMILD_IMPLEMENT_RTTI( crimild::ShaderOutlet )
		
	public:
		enum class Type {
			ANY,
			SCALAR,
			VECTOR_2,
			VECTOR_3,
			VECTOR_4,
			MATRIX_3,
			MATRIX_4,
			SAMPLER_2D,
		};
		
	public:
		explicit ShaderOutlet( std::string name, Type type, crimild::Int32 priority )
			: NamedObject( name ),
			  _type( type ),
			  _priority( priority )
		{
			std::stringstream ss;
			ss << name << "_" << getUniqueID();
			_uniqueName = ss.str();
		}

		virtual ~ShaderOutlet( void )
		{

		}

		Type getType( void ) const
		{
			if ( _type != Type::ANY || !isConnected() ) {
				return _type;
			}
			else {
				return _connectedOutlet->_type;
			}
		}

		crimild::Int32 getPriority( void ) const { return _priority; }

		std::string getUniqueName( void ) const { return _uniqueName; }

	private:
		Type _type;
		std::string _uniqueName;
		crimild::Int32 _priority;

	public:
		crimild::Bool isConnected( void ) const
		{
			return _connectedOutlet != nullptr;
		}
		
		ShaderNode *getConnectedNode( void ) { return _connectedNode; }
		ShaderOutlet *getConnectedOutlet( void ) { return _connectedOutlet; }

		/**
		   \remarks Internal use only
		 */
		void connect( SharedPointer< ShaderNode > const &node, SharedPointer< ShaderOutlet > const &outlet )
		{
			_connectedNode = crimild::get_ptr( node );
			_connectedOutlet = crimild::get_ptr( outlet );
		}

	private:
		ShaderNode *_connectedNode = nullptr;
		ShaderOutlet *_connectedOutlet = nullptr;
	};

	class ShaderNode : public coding::Codable {
	private:
		using OutletMap = containers::Map< std::string, SharedPointer< ShaderOutlet >>;
		using OutletArray = containers::Array< SharedPointer< ShaderOutlet >>;
		
	protected:
		explicit ShaderNode( std::string description = "" )
			: _description( description )
		{

		}

	public:
		virtual ~ShaderNode( void )
		{

		}

		const std::string &getDescription( void ) const { return _description; }

	private:
		std::string _description;

	public:
		OutletMap &getInputs( void ) { return _inputs; }

		void setInputs( OutletArray const &inputs )
		{
			inputs.each( [ this ]( SharedPointer< ShaderOutlet > const &outlet ) {
				_inputs[ outlet->getName() ] = outlet;
			});
		}
		
		OutletMap &getOutputs( void ) { return _outputs; }

		void setOutputs( OutletArray const &outputs )
		{
			outputs.each( [ this ]( SharedPointer< ShaderOutlet > const &outlet ) {
				_outputs[ outlet->getName() ] = outlet;
			});
		}

	private:
		containers::Map< std::string, SharedPointer< ShaderOutlet >> _inputs;
		containers::Map< std::string, SharedPointer< ShaderOutlet >> _outputs;
	};

	class ShaderInputNode : public ShaderNode {
		CRIMILD_IMPLEMENT_RTTI( crimild::ShaderInputNode )
		
	public:
		explicit ShaderInputNode( std::string description = "" ) : ShaderNode( description ) { }
		virtual ~ShaderInputNode( void ) { }
	};

	class ShaderUniformNode : public ShaderNode {
		CRIMILD_IMPLEMENT_RTTI( crimild::ShaderUniformNode )
		
	public:
		explicit ShaderUniformNode( std::string description = "" ) : ShaderNode( description ) { }
		virtual ~ShaderUniformNode( void ) { }
	};

	class ShaderOutputNode : public ShaderNode {
		CRIMILD_IMPLEMENT_RTTI( crimild::ShaderOutputNode )
		
	public:
		explicit ShaderOutputNode( std::string description = "" ) : ShaderNode( description ) { }
		virtual ~ShaderOutputNode( void ) { }
	};

	class ShaderGraph : public coding::Codable {
		CRIMILD_IMPLEMENT_RTTI( crimild::ShaderGraph )

	private:
		using ShaderNodeArray = containers::Array< SharedPointer< ShaderNode >>;

	public:
		enum class Type {
			VERTEX,
			FRAGMENT
		};
		
	public:
		explicit ShaderGraph( Type type )
			: _type( type )
		{

		}

		virtual ~ShaderGraph( void )
		{

		}

		Type getType( void ) const { return _type; }

	private:
		Type _type;

	public:
        template< typename T, typename... Args >
		SharedPointer< T > createNode( Args &&... args )
		{
			auto node = crimild::alloc< T >( std::forward< Args >( args )... );
			_nodes.add( node );
			_sinkNodes.add( node );
			_sourceNodes.add( node );
			return node;
		}

		ShaderNodeArray &getNodes( void ) { return _nodes; }
		ShaderNodeArray &getSourceNodes( void ) { return _sourceNodes; }
		ShaderNodeArray &getSinkNodes( void ) { return _sinkNodes; }

		bool connect( SharedPointer< ShaderNode > const &a, std::string outletA, SharedPointer< ShaderNode > const &b, std::string outletB )
		{
			assert( a != nullptr );
			assert( b != nullptr );
			
			auto aOutput = a->getOutputs()[ outletA ];
			if ( aOutput == nullptr ) {
				throw RuntimeException( "No output outlet with name " + outletA + " in node " + a->getClassName() );
			}
			
			auto bInput = b->getInputs()[ outletB ];
			if ( bInput == nullptr ) {
				throw RuntimeException( "No input outlet with name " + outletB + " in node " + b->getClassName() );
			}

			aOutput->connect( b, bInput );
			bInput->connect( a, aOutput );
			
			if ( !_nodes.contains( a ) ) {
				_nodes.add( a );
				_sourceNodes.add( a );
			}
			
			if ( _sinkNodes.contains( a ) ) {
				_sinkNodes.remove( a );
			}

			if ( !_nodes.contains( b ) ) {
				_nodes.add( b );
				_sinkNodes.add( b );
			}
			
			if ( _sourceNodes.contains( b ) ) {
				_sourceNodes.remove( b );
			}

			return true;
		}

		crimild::Size indegree( SharedPointer< ShaderNode > const &node )
		{
			crimild::Size count = 0;
			node->getInputs().eachValue( [ &count ]( SharedPointer< ShaderOutlet > const &outlet ) {
				if ( outlet->isConnected() ) {
					++count;
				}
			});
			return count;
		}

		crimild::Size outdegree( SharedPointer< ShaderNode > const &node )
		{
			crimild::Size count = 0;
			node->getOutputs().eachValue( [ &count ]( SharedPointer< ShaderOutlet > const &outlet ) {
				if ( outlet->isConnected() ) {
					++count;
				}
			});
			return count;
		}

	private:
		ShaderNodeArray _nodes;
		ShaderNodeArray _sinkNodes;
		ShaderNodeArray _sourceNodes;

	public:
		SharedPointer< ShaderNode > getStandardVertexShaderInputs( void )
		{
			auto node = crimild::alloc< ShaderInputNode >( "Standard Vertex Shader Inputs" );
			node->setOutputs({
				crimild::alloc< ShaderOutlet >( StandardShaderInputs::VERTEX_POSITION, ShaderOutlet::Type::VECTOR_3, 0 ),
				crimild::alloc< ShaderOutlet >( StandardShaderInputs::VERTEX_COLOR, ShaderOutlet::Type::VECTOR_4, 1 ),
			});			
			return node;
		}

		SharedPointer< ShaderNode > getStandardShaderUniforms( void )
		{
			auto node = crimild::alloc< ShaderUniformNode >( "Standard uniforms" );
			node->setOutputs({
				crimild::alloc< ShaderOutlet >( StandardShaderUniforms::MODEL_MATRIX, ShaderOutlet::Type::MATRIX_4, 0 ),
				crimild::alloc< ShaderOutlet >( StandardShaderUniforms::VIEW_MATRIX, ShaderOutlet::Type::MATRIX_4, 1 ),
				crimild::alloc< ShaderOutlet >( StandardShaderUniforms::PROJECTION_MATRIX, ShaderOutlet::Type::MATRIX_4, 2 ),
			});			
			return node;
		}

		SharedPointer< ShaderNode > getStandardVertexShaderOutputs( void )
		{
			auto node = crimild::alloc< ShaderOutputNode >( "Standard Vertex Shader Outputs" );
			node->setInputs({
				crimild::alloc< ShaderOutlet >( StandardShaderOutputs::VERTEX_SCREEN_POSITION, ShaderOutlet::Type::VECTOR_4, 0 ),
				crimild::alloc< ShaderOutlet >( StandardShaderOutputs::VERTEX_COLOR, ShaderOutlet::Type::VECTOR_4, 1 ),
			});
			return node;
		}
	};

	class ShaderBuilder;

	class ShaderNodeTranslator : public coding::Codable {
	protected:
		ShaderNodeTranslator( void ) { }
		
	public:
		virtual ~ShaderNodeTranslator( void ) { }
		
		virtual void translate( SharedPointer< ShaderGraph > const &graph, SharedPointer< ShaderNode > const &node, ShaderBuilder *builder ) = 0;		
	};

	class ShaderBuilder : public SharedObject {
	public:
		ShaderBuilder( void )
		{
			
		}

		virtual ~ShaderBuilder( void )
		{
			
		}

	public:
		template< typename SHADER_NODE_TYPE, typename TRANSLATOR_TYPE >
		void registerTranslator( void )
		{
			_translators[ SHADER_NODE_TYPE::__CLASS_NAME ] = crimild::alloc< TRANSLATOR_TYPE >();
		}

	private:
		containers::Map< std::string, SharedPointer< ShaderNodeTranslator >> _translators;

	public:
		SharedPointer< Shader > build( SharedPointer< ShaderGraph > const &graph )
		{
			containers::List< SharedPointer< ShaderNode >> frontier;
			containers::List< SharedPointer< ShaderNode >> explored;
			containers::Map< coding::Codable::UniqueID, crimild::Int32 > nodePrereqs;

			graph->getNodes().each( [ &nodePrereqs, &frontier, graph ]( SharedPointer< ShaderNode > &node ) {
				auto prereqs = graph->indegree( node );
				if ( prereqs > 0 ) {
					nodePrereqs[ node->getUniqueID() ] = prereqs;
				}
				else {
					frontier.add( node );
				}
			});

			while ( !frontier.empty() ) {
				auto node = frontier.first();
				frontier.remove( node );
				explored.add( node );
				
				node->getOutputs().eachValue( [ graph, node, &frontier, &explored, &nodePrereqs ]( SharedPointer< ShaderOutlet > const &outlet ) {
					if ( outlet->isConnected() ) {
						auto other = crimild::retain( outlet->getConnectedNode() );
						if ( explored.contains( other ) || frontier.contains( other ) ) {
							// loops not allowed
							std::string msg = "Shader Graph contains a loop: ";
							msg += node->getDescription();
							msg += " attempted to push " + other->getDescription();
							throw RuntimeException( msg );
						}
						else {
							nodePrereqs[ other->getUniqueID() ] -= 1;
							if ( nodePrereqs[ other->getUniqueID() ] == 0 ) {
								frontier.add( other );
							}
						}
					}
				});
			}

			explored.each( [ this, graph ]( SharedPointer< ShaderNode > const &node ) {
				if ( auto translator = _translators[ node->getClassName() ] ) {
					translator->translate( graph, node, this );
				}
			});

			std::string source = generateShaderSource();

			return crimild::alloc< Shader >( source );
		}

	public:
		struct ShaderSection {
			containers::Array< std::string > lines;
		};

		ShaderSection &getInputsSection( void ) { return _inputsSection; }
		ShaderSection &getUniformsSection( void ) { return _uniformsSection; }
		ShaderSection &getOutputsSection( void ) { return _outputsSection; }
		ShaderSection &getGlobalsSection( void ) { return _globalsSection; }
		ShaderSection &getMainSection( void ) { return _mainSection; }
		
	private:
		ShaderSection _inputsSection;
		ShaderSection _uniformsSection;
		ShaderSection _outputsSection;
		ShaderSection _globalsSection;
		ShaderSection _mainSection;

	protected:
		virtual std::string generateShaderSource( void ) = 0;
	};

	/*
	class ShaderChunk : public SharedObject {
	public:
	};

	class FresnelShaderNode : public ShaderNode {
		CRIMILD_IMPLEMENT_RTTI( crimild::FresnelShaderNode )

	public:
		FresnelShaderNode( void ) { }
		virtual ~FresnelShaderNode( void ) { }
	};
	*/

	class MultiplyNode : public ShaderNode {
		CRIMILD_IMPLEMENT_RTTI( crimild::MultiplyShaderNode )

	public:
		class Inputs {
		public:
			static const char *A;
			static const char *B;
		};

		class Outputs {
		public:
			static const char *RESULT;
		};

	public:
		explicit MultiplyNode( ShaderOutlet::Type aType, ShaderOutlet::Type bType, ShaderOutlet::Type retType, std::string description = "" )
			: ShaderNode( description )
		{
			setInputs({
				crimild::alloc< ShaderOutlet >( Inputs::A, aType, 0 ),
				crimild::alloc< ShaderOutlet >( Inputs::B, bType, 1 ),
			});

			setOutputs({
				crimild::alloc< ShaderOutlet >( Outputs::RESULT, retType, 0 ),
			});
		}

		virtual ~MultiplyNode( void )
		{

		}
	};

	/**
	   \remarks Value is used if no input is specified
	 */
	class VectorNode : public ShaderNode {
		CRIMILD_IMPLEMENT_RTTI( crimild::VectorNode )
		
	public:
		class Inputs {
		public:
			static const char *X;
			static const char *Y;
			static const char *Z;
			static const char *W;
			static const char *XY;
			static const char *XYZ;
			static const char *XYZW;
		};
		
		class Outputs {
		public:
			static const char *X;
			static const char *Y;
			static const char *Z;
			static const char *W;
			static const char *XY;
			static const char *XYZ;
			static const char *XYZW;
		};
		
	public:
		explicit VectorNode( std::string description = "" )
			: VectorNode( Vector4f::ZERO, description )
		{
			setInputs({
				crimild::alloc< ShaderOutlet >( Inputs::X, ShaderOutlet::Type::SCALAR, 0 ),				
				crimild::alloc< ShaderOutlet >( Inputs::Y, ShaderOutlet::Type::SCALAR, 1 ),				
				crimild::alloc< ShaderOutlet >( Inputs::Z, ShaderOutlet::Type::SCALAR, 2 ),				
				crimild::alloc< ShaderOutlet >( Inputs::XY, ShaderOutlet::Type::VECTOR_2, 3 ),				
				crimild::alloc< ShaderOutlet >( Inputs::XYZ, ShaderOutlet::Type::VECTOR_3, 4 ),				
				crimild::alloc< ShaderOutlet >( Inputs::XYZW, ShaderOutlet::Type::VECTOR_4, 5 ),				
			});

			setOutputs({
				crimild::alloc< ShaderOutlet >( Outputs::X, ShaderOutlet::Type::SCALAR, 0 ),				
				crimild::alloc< ShaderOutlet >( Outputs::Y, ShaderOutlet::Type::SCALAR, 1 ),				
				crimild::alloc< ShaderOutlet >( Outputs::Z, ShaderOutlet::Type::SCALAR, 2 ),				
				crimild::alloc< ShaderOutlet >( Outputs::XY, ShaderOutlet::Type::VECTOR_2, 3 ),				
				crimild::alloc< ShaderOutlet >( Outputs::XYZ, ShaderOutlet::Type::VECTOR_3, 4 ),				
				crimild::alloc< ShaderOutlet >( Outputs::XYZW, ShaderOutlet::Type::VECTOR_4, 5 ),				
			});
		}

		explicit VectorNode( const Vector4f &value, std::string description = "" )
			: ShaderNode( description ),
			  _type( ShaderOutlet::Type::VECTOR_4 ),
			  _value( value )
		{

		}

		ShaderOutlet::Type getType( void ) const { return _type; }
		const Vector4f &getValue( void ) const { return _value; }

	private:
		ShaderOutlet::Type _type;
		Vector4f _value;
	};

	namespace opengl {

		class OpenGLShaderNodeTranslator : public ShaderNodeTranslator {
		protected:
			OpenGLShaderNodeTranslator( void ) { }
			
		public:
			virtual ~OpenGLShaderNodeTranslator( void ) { }

		protected:
			std::string getOutletTypeStr( SharedPointer< ShaderOutlet > const &outlet )
			{
				auto type = outlet->getType();
				std::string typeStr;
				switch ( type ) {
				case ShaderOutlet::Type::SCALAR:
					typeStr = "float";
					break;
					
				case ShaderOutlet::Type::VECTOR_2:
					typeStr = "vec2";
					break;
					
				case ShaderOutlet::Type::VECTOR_3:
					typeStr = "vec3";
					break;
					
				case ShaderOutlet::Type::VECTOR_4:
					typeStr = "vec4";
					break;

				case ShaderOutlet::Type::MATRIX_3:
					typeStr = "mat3";
					break;
					
				case ShaderOutlet::Type::MATRIX_4:
					typeStr = "mat4";
					break;
					
				default:
					typeStr = "unknown";
					break;
				}
				
				return typeStr;
			}
		};

		class ShaderInputNodeTranslator : public OpenGLShaderNodeTranslator {
			CRIMILD_IMPLEMENT_RTTI( crimild::opengl::ShaderInputNodeTranslator )

		public:
			virtual void translate( SharedPointer< ShaderGraph > const &graph, SharedPointer< ShaderNode > const &node, ShaderBuilder *builder ) override
			{
				node->getOutputs().eachValue( [ this, builder ]( SharedPointer< ShaderOutlet > const &outlet ) {
					if ( outlet->isConnected() ) {
						auto attribType = getOutletTypeStr( outlet );
						builder->getInputsSection().lines.add( "in " + attribType + " " + outlet->getUniqueName() + ";" );
					}
				});
			}
		};

		class ShaderUniformNodeTranslator : public OpenGLShaderNodeTranslator {
			CRIMILD_IMPLEMENT_RTTI( crimild::opengl::ShaderUniformNodeTranslator )

		public:
			virtual void translate( SharedPointer< ShaderGraph > const &graph, SharedPointer< ShaderNode > const &node, ShaderBuilder *builder ) override
			{
				node->getOutputs().eachValue( [ this, builder, node ]( SharedPointer< ShaderOutlet > const &outlet ) {
					if ( outlet->isConnected() ) {
						auto attribType = getOutletTypeStr( outlet );
						builder->getUniformsSection().lines.add( "uniform " + attribType + " " + outlet->getUniqueName() + ";" );
					}
				});
			}
		};

		class ShaderOutputNodeTranslator : public OpenGLShaderNodeTranslator {
			CRIMILD_IMPLEMENT_RTTI( crimild::opengl::ShaderOutputNodeTranslator )

		public:
			virtual void translate( SharedPointer< ShaderGraph > const &graph, SharedPointer< ShaderNode > const &node, ShaderBuilder *builder ) override
			{
				node->getInputs().eachValue( [ this, builder, node ]( SharedPointer< ShaderOutlet > const &outlet ) {
					if ( outlet->isConnected() ) {
						auto otherOutlet = outlet->getConnectedOutlet();
						if ( outlet->getName() == StandardShaderOutputs::VERTEX_SCREEN_POSITION ) {
							builder->getMainSection().lines.add( "gl_Position = " + otherOutlet->getUniqueName() + ";" );
						}
						else {
							auto attribType = getOutletTypeStr( outlet );
							builder->getOutputsSection().lines.add( "out " + attribType + " " + outlet->getUniqueName() + ";" );
							builder->getMainSection().lines.add( outlet->getUniqueName() + " = " + otherOutlet->getUniqueName() + ";" );
						}
					}
				});
			}
		};

		class MultiplyNodeTranslator : public OpenGLShaderNodeTranslator {
			CRIMILD_IMPLEMENT_RTTI( crimild::opengl::MultiplyNodeTranslator )
			
		public:
			virtual void translate( SharedPointer< ShaderGraph > const &graph, SharedPointer< ShaderNode > const &node, ShaderBuilder *builder ) override
			{
				auto inA = node->getInputs()[ MultiplyNode::Inputs::A ]->getConnectedOutlet();
				auto inB = node->getInputs()[ MultiplyNode::Inputs::B ]->getConnectedOutlet();
				auto out = node->getOutputs()[ MultiplyNode::Outputs::RESULT ];

				if ( inA == nullptr || inB == nullptr || out == nullptr ) {
					Log::error( CRIMILD_CURRENT_CLASS_NAME, "Insufficient arguments for node ", node->getClassName() );
					return;
				}
				
				auto a = inA->getUniqueName();
				auto b = inB->getUniqueName();
				auto ret = out->getUniqueName();
				auto retType = getOutletTypeStr( out );

				auto line = ( retType + " " + ret + " = " + a + " * " + b + ";" );
				
				builder->getMainSection().lines.add( line );
			}
		};

		class VectorNodeTranslator : public OpenGLShaderNodeTranslator {
			CRIMILD_IMPLEMENT_RTTI( crimild::opengl::VectorNodeTranslator )
			
		public:
			virtual void translate( SharedPointer< ShaderGraph > const &graph, SharedPointer< ShaderNode > const &node, ShaderBuilder *builder ) override
			{
				SharedPointer< ShaderOutlet > out;
				node->getOutputs().eachValue( [ &out ]( SharedPointer< ShaderOutlet > const &outlet ) {
					if ( outlet->isConnected() ) {
						out = outlet;
					}
				});

				std::string inStr = "vec4()";

				if ( out->getName() == VectorNode::Outputs::XYZW ) {
					if ( auto in = node->getInputs()[ VectorNode::Inputs::XYZW ]->getConnectedOutlet() ) {
						inStr = in->getUniqueName();
					}
					else if ( auto in = node->getInputs()[ VectorNode::Inputs::XYZ ]->getConnectedOutlet() ) {
						inStr = "vec4( " + in->getUniqueName() + ", 1.0 )";
					}
				}

				builder->getMainSection().lines.add( getOutletTypeStr( out ) + " " + out->getUniqueName() + " = " + inStr + ";" );
			}
		};

	/*
		class FresnelShaderChunk : public ShaderChunk {
		public:
			virtual std::string getSource( ShaderNode *node ) const
			{
				auto normal = node->getInput( "normal" )->getId();
				auto eye = node->getInput( "eye" )->getId();
				auto result = node->getOutput( "result" )->getId();

				return result + " = dot( " + normal + ", " + eye + " );";
			}
		};
	*/

		class OpenGLShaderBuilder : public ShaderBuilder {
		public:
			OpenGLShaderBuilder( void )
			{
				registerTranslator< ShaderInputNode, ShaderInputNodeTranslator >();
				registerTranslator< ShaderUniformNode, ShaderUniformNodeTranslator >();
				registerTranslator< ShaderOutputNode, ShaderOutputNodeTranslator >();
				registerTranslator< MultiplyNode, MultiplyNodeTranslator >();
				registerTranslator< VectorNode, VectorNodeTranslator >();
			}

			virtual ~OpenGLShaderBuilder( void )
			{
				
			}

		protected:
			virtual std::string generateShaderSource( void ) override
			{
				std::stringstream ss;

				ss << "#version 400\n";

				ss << "\n// Inputs";
				getInputsSection().lines.each( [ &ss ]( std::string &line ) {
					ss << "\n" << line;
				});
				ss << "\n";

				ss << "\n// Uniforms";
				getUniformsSection().lines.each( [ &ss ]( std::string &line ) {
					ss << "\n" << line;
				});
				ss << "\n";

				ss << "\n// Outputs";
				getOutputsSection().lines.each( [ &ss ]( std::string &line ) {
					ss << "\n" << line;
				});
				ss << "\n";

				ss << "\nvoid main()\n{";
				getMainSection().lines.each( [ &ss ]( std::string &line ) {
					ss << "\n\t" << line;
				});
				ss << "\n}\n";

				return ss.str();
			}
		};

	}

}

using namespace crimild;

const char *StandardShaderInputs::VERTEX_POSITION = "inVertPosition";
const char *StandardShaderInputs::VERTEX_COLOR = "inVertColor";

const char *StandardShaderUniforms::MODEL_MATRIX = "uMMatrix";
const char *StandardShaderUniforms::VIEW_MATRIX = "uVMatrix";
const char *StandardShaderUniforms::PROJECTION_MATRIX = "uPMatrix";

const char *StandardShaderOutputs::VERTEX_SCREEN_POSITION = "outScreenPosition";
const char *StandardShaderOutputs::VERTEX_COLOR = "outVertColor";

const char *VectorNode::Inputs::X = "vec_x";
const char *VectorNode::Inputs::Y = "vec_y";
const char *VectorNode::Inputs::Z = "vec_z";
const char *VectorNode::Inputs::XY = "vec_xy";
const char *VectorNode::Inputs::XYZ = "vec_xyz";
const char *VectorNode::Inputs::XYZW = "vec_xyzw";

const char *VectorNode::Outputs::X = "vec_x";
const char *VectorNode::Outputs::Y = "vec_y";
const char *VectorNode::Outputs::Z = "vec_z";
const char *VectorNode::Outputs::XY = "vec_xy";
const char *VectorNode::Outputs::XYZ = "vec_xyz";
const char *VectorNode::Outputs::XYZW = "vec_xyzw";

const char *MultiplyNode::Inputs::A = "mult_a";
const char *MultiplyNode::Inputs::B = "mult_b";
const char *MultiplyNode::Outputs::RESULT = "mult_ret";

int main( int argc, char **argv )
{
   auto sim = crimild::alloc< GLSimulation >( "Shader Graph", crimild::alloc< Settings >( argc, argv ) );
   sim->getRenderer()->getScreenBuffer()->setClearColor( RGBAColorf( 0.5f, 0.5f, 0.5f, 1.0f ) );

    auto scene = crimild::alloc< Group >();

    float vertices[] = {
        -1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f
    };

    unsigned short indices[] = {
        0, 1, 2
    };

    auto primitive = crimild::alloc< Primitive >();
    primitive->setVertexBuffer( crimild::alloc< VertexBufferObject >( VertexFormat::VF_P3_C4, 3, vertices ) );
    primitive->setIndexBuffer( crimild::alloc< IndexBufferObject >( 3, indices ) );

    auto geometry = crimild::alloc< Geometry >();
    geometry->attachPrimitive( primitive );
    auto material = crimild::alloc< Material >();
    //material->setProgram( AssetManager::getInstance()->get< ShaderProgram >( Renderer::SHADER_PROGRAM_UNLIT_VERTEX_COLOR ) );
    material->getCullFaceState()->setEnabled( false );
    geometry->getComponent< MaterialComponent >()->attachMaterial( material );
    geometry->attachComponent< RotationComponent >( Vector3f( 0.0f, 1.0f, 0.0f ), 0.25f * Numericf::HALF_PI );
    scene->attachNode( geometry );

	auto vsGraph = crimild::alloc< ShaderGraph >( ShaderGraph::Type::VERTEX );
	
	auto attributes = vsGraph->getStandardVertexShaderInputs();
	auto uniforms = vsGraph->getStandardShaderUniforms();

	auto position = vsGraph->createNode< VectorNode >( "Homogenous Vertex Position" );
	vsGraph->connect( attributes, StandardShaderInputs::VERTEX_POSITION, position, VectorNode::Inputs::XYZ );

	auto worldPosition = vsGraph->createNode< MultiplyNode >( ShaderOutlet::Type::MATRIX_4, ShaderOutlet::Type::VECTOR_4, ShaderOutlet::Type::VECTOR_4, "Compute World-Space Position" );
	vsGraph->connect( uniforms, StandardShaderUniforms::MODEL_MATRIX, worldPosition, MultiplyNode::Inputs::A );
	vsGraph->connect( position, VectorNode::Outputs::XYZW, worldPosition, MultiplyNode::Inputs::B );

	auto viewPosition = vsGraph->createNode< MultiplyNode >( ShaderOutlet::Type::MATRIX_4, ShaderOutlet::Type::VECTOR_4, ShaderOutlet::Type::VECTOR_4, "Compute View-Space Position" );
	vsGraph->connect( uniforms, StandardShaderUniforms::VIEW_MATRIX, viewPosition, MultiplyNode::Inputs::A );
	vsGraph->connect( worldPosition, MultiplyNode::Outputs::RESULT, viewPosition, MultiplyNode::Inputs::B );

	auto screenPosition = vsGraph->createNode< MultiplyNode >( ShaderOutlet::Type::MATRIX_4, ShaderOutlet::Type::VECTOR_4, ShaderOutlet::Type::VECTOR_4, "Compute Screen-Space Position" );
	vsGraph->connect( uniforms, StandardShaderUniforms::PROJECTION_MATRIX, screenPosition, MultiplyNode::Inputs::A );
	vsGraph->connect( viewPosition, MultiplyNode::Outputs::RESULT, screenPosition, MultiplyNode::Inputs::B );

	auto outputs = vsGraph->getStandardVertexShaderOutputs();
	vsGraph->connect( screenPosition, MultiplyNode::Outputs::RESULT, outputs, StandardShaderOutputs::VERTEX_SCREEN_POSITION );
	vsGraph->connect( attributes, StandardShaderInputs::VERTEX_COLOR, outputs, StandardShaderOutputs::VERTEX_COLOR );

	auto vsShaderBuilder = crimild::alloc< opengl::OpenGLShaderBuilder >();
	auto vertexShader = vsShaderBuilder->build( vsGraph );

	std::cout << vertexShader->getSource() << std::endl;

	const char *unlit_vertex_color_fs = { CRIMILD_TO_STRING(
			
			CRIMILD_GLSL_PRECISION_FLOAT_HIGH
			
			CRIMILD_GLSL_VARYING_IN vec4 vColor;
			
			CRIMILD_GLSL_DECLARE_FRAGMENT_OUTPUT
			
			void main( void ) 
			{
				CRIMILD_GLSL_FRAGMENT_OUTPUT = vColor;
			}
			)};

	auto fsSource = StringUtils::replaceAll(
		std::string( unlit_vertex_color_fs ),
		"vColor",
		outputs->getInputs()[ StandardShaderOutputs::VERTEX_COLOR ]->getUniqueName() );

	std::cout << "\n" << fsSource << std::endl;

	auto program = crimild::alloc< ShaderProgram >( vertexShader, opengl::OpenGLUtils::getFragmentShaderInstance( fsSource ) );
	program->registerStandardLocation(
		ShaderLocation::Type::ATTRIBUTE,
		ShaderProgram::StandardLocation::POSITION_ATTRIBUTE,
		attributes->getOutputs()[ StandardShaderInputs::VERTEX_POSITION ]->getUniqueName() );
	program->registerStandardLocation(
		ShaderLocation::Type::ATTRIBUTE,
		ShaderProgram::StandardLocation::COLOR_ATTRIBUTE,
		attributes->getOutputs()[ StandardShaderInputs::VERTEX_COLOR ]->getUniqueName() );
	program->registerStandardLocation(
		ShaderLocation::Type::UNIFORM,
		ShaderProgram::StandardLocation::PROJECTION_MATRIX_UNIFORM,
		uniforms->getOutputs()[ StandardShaderUniforms::PROJECTION_MATRIX ]->getUniqueName() );
	program->registerStandardLocation(
		ShaderLocation::Type::UNIFORM,
		ShaderProgram::StandardLocation::VIEW_MATRIX_UNIFORM,
		uniforms->getOutputs()[ StandardShaderUniforms::VIEW_MATRIX ]->getUniqueName() );
	program->registerStandardLocation(
		ShaderLocation::Type::UNIFORM,
		ShaderProgram::StandardLocation::MODEL_MATRIX_UNIFORM,
		uniforms->getOutputs()[ StandardShaderUniforms::MODEL_MATRIX ]->getUniqueName() );
	
	material->setProgram( program );
	
    auto camera = crimild::alloc< Camera >();
    camera->local().setTranslate( Vector3f( 0.0f, 0.0f, 3.0f ) );
    scene->attachNode( camera );

    sim->setScene( scene );
	return sim->run();
}

