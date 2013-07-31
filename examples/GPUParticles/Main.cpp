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

#include <fstream>
#include <string>
#include <vector>

using namespace crimild;

const char *particle_system_vs = { CRIMILD_TO_STRING(
	struct Shape {
		vec3 center;
		float radius;
	};

	in vec3 aPosition;
	in vec3 aNormal;   // used for initial velocity
	in vec3 aTextureCoord;   // tc0 = lifeTime, tc1 = unused

	uniform mat4 uPMatrix;
	uniform mat4 uVMatrix;
	uniform mat4 uMMatrix;

	uniform float uTime;
	uniform float uLifeTime;
	uniform vec3 uGravity;
	uniform Shape uShape;
	uniform bool uHasShape;

	out float vParticleTime;

	void main(void) {
	    float size = aTextureCoord[ 0 ];
	    float offset = aTextureCoord[ 1 ];
	    vParticleTime = uTime + offset;
	    vParticleTime = vParticleTime - uLifeTime * floor( vParticleTime / uLifeTime );
	    vec4 worldGravity = inverse( uMMatrix ) * vec4( uGravity, 0.0 );
	    vec3 position = aPosition + aNormal * vParticleTime + 0.5 * vParticleTime * vParticleTime * worldGravity.xyz;

	    vec4 worldPosition = uMMatrix * vec4( position, 1.0 );
	    //if ( uHasShape ) {
		    vec3 diff = worldPosition.xyz - uShape.center;
		    float d = distance( worldPosition.xyz , uShape.center );
		    if ( d < uShape.radius ) {
		    	worldPosition.xyz = uShape.center + uShape.radius * normalize( diff );
		    }
		//}

	    gl_Position = uPMatrix * uVMatrix * worldPosition;
	    gl_PointSize = ( uLifeTime - vParticleTime ) * size;
	}
)};

const char *particle_system_fs = { CRIMILD_TO_STRING( 
	struct Material {
	    vec4 ambient;
	    vec4 diffuse;
	    vec4 specular;
	    float shininess;
	};

	in float vParticleTime;

	uniform Material uMaterial; 
	uniform sampler2D uColorMap;
	uniform bool uUseColorMap;

	uniform float uLifeTime;

	out vec4 vFragColor;

	void main( void ) 
	{ 
    	vec4 color = uUseColorMap ? texture( uColorMap, gl_PointCoord ) : uMaterial.diffuse;
    	color *= uMaterial.diffuse;
    	if ( color.a == 0.0 ) {
    		discard;
    	}
		vFragColor = color; 
	}
)};

class ParticleSystemShaderProgram : public ShaderProgram {
public:
	ParticleSystemShaderProgram( void );
	virtual ~ParticleSystemShaderProgram( void );
};

typedef std::shared_ptr< ParticleSystemShaderProgram > ParticleSystemShaderProgramPtr;

ParticleSystemShaderProgram::ParticleSystemShaderProgram( void )
	: ShaderProgram( gl3::Utils::getVertexShaderInstance( particle_system_vs ), gl3::Utils::getFragmentShaderInstance( particle_system_fs ) )
{ 
	registerStandardLocation( ShaderLocation::Type::ATTRIBUTE, ShaderProgram::StandardLocation::POSITION_ATTRIBUTE, "aPosition" );
	registerStandardLocation( ShaderLocation::Type::ATTRIBUTE, ShaderProgram::StandardLocation::NORMAL_ATTRIBUTE, "aNormal" );
	registerStandardLocation( ShaderLocation::Type::ATTRIBUTE, ShaderProgram::StandardLocation::TEXTURE_COORD_ATTRIBUTE, "aTextureCoord" );

	registerStandardLocation( ShaderLocation::Type::UNIFORM, ShaderProgram::StandardLocation::PROJECTION_MATRIX_UNIFORM, "uPMatrix" );
	registerStandardLocation( ShaderLocation::Type::UNIFORM, ShaderProgram::StandardLocation::VIEW_MATRIX_UNIFORM, "uVMatrix" );
	registerStandardLocation( ShaderLocation::Type::UNIFORM, ShaderProgram::StandardLocation::MODEL_MATRIX_UNIFORM, "uMMatrix" );

	registerStandardLocation( ShaderLocation::Type::UNIFORM, ShaderProgram::StandardLocation::MATERIAL_AMBIENT_UNIFORM, "uMaterial.ambient" );
	registerStandardLocation( ShaderLocation::Type::UNIFORM, ShaderProgram::StandardLocation::MATERIAL_DIFFUSE_UNIFORM, "uMaterial.diffuse" );
	registerStandardLocation( ShaderLocation::Type::UNIFORM, ShaderProgram::StandardLocation::MATERIAL_SPECULAR_UNIFORM, "uMaterial.specular" );
	registerStandardLocation( ShaderLocation::Type::UNIFORM, ShaderProgram::StandardLocation::MATERIAL_SHININESS_UNIFORM, "uMaterial.shininess" );

	registerStandardLocation( ShaderLocation::Type::UNIFORM, ShaderProgram::StandardLocation::MATERIAL_COLOR_MAP_UNIFORM, "uColorMap" );
	registerStandardLocation( ShaderLocation::Type::UNIFORM, ShaderProgram::StandardLocation::MATERIAL_USE_COLOR_MAP_UNIFORM, "uUseColorMap" );
}

ParticleSystemShaderProgram::~ParticleSystemShaderProgram( void )
{ 
}

class ParticleSystemComponent : public NodeComponent {
	CRIMILD_DISALLOW_COPY_AND_ASSIGN( ParticleSystemComponent );

public:
	static const char *NAME;

public:
	ParticleSystemComponent( void );

	virtual ~ParticleSystemComponent( void );

	virtual void onAttach( void ) override;

	virtual void update( const Time &t ) override;

	void setParticleCount( unsigned short value ) { _particleCount = value; }
	unsigned short getParticleCount( void ) const { return _particleCount; }

	void setParticleSize( float value ) { _particleSize = value; }
	float getParticleSize( void ) const { return _particleSize; }

	void setParticleDuration( float value ) { _particleDuration = value; }
	float getParticleDuration( void ) const { return _particleDuration; }

	void setGravity( const Vector3f &value ) { _gravity = value; }
	const Vector3f &getGravity( void ) const { return _gravity; }

	void setSpread( const Vector3f &value ) { _spread = value; }
	const Vector3f &getSpread( void ) const { return _spread; }

	void setVelocity( const Vector3f &value ) { _velocity = value; }
	const Vector3f &getVelocity( void ) const { return _velocity; }

	void setLooping( float value ) { _looping = value; }
	float getLooping( void ) const { return _looping; }

	Material *getParticleMaterial( void ) { return _material.get(); }

	void setShape( BoundingVolume *volume ) { _shape = volume; }
	BoundingVolume *getShape( void ) { return _shape; }

	void generateParticles( void );

private:
	PrimitivePtr _primitive;
	MaterialPtr _material;

	unsigned short _particleCount;
	float _particleSize;
	float _particleDuration;
	Vector3f _gravity;
	Vector3f _velocity;
	Vector3f _spread;
	bool _looping;
	BoundingVolume *_shape;

	Vector3fUniformPtr _gravityUniform;
	FloatUniformPtr _timeUniform;
	FloatUniformPtr _durationUniform;
	FloatUniformPtr _shapeRadiusUniform;
	Vector3fUniformPtr _shapeCenterUniform;
	BoolUniformPtr _shapeFlagUniform;
};

typedef std::shared_ptr< ParticleSystemComponent > ParticleSystemComponentPtr;

class ParticleSystem : public Geometry {
public:
	ParticleSystem( std::string name = "" );
	virtual ~ParticleSystem( void );
};

typedef std::shared_ptr< ParticleSystem > ParticleSystemPtr;

ParticleSystem::ParticleSystem( std::string name )
	: Geometry( name )
{
	ParticleSystemComponentPtr particleSystemComponent( new ParticleSystemComponent() );
	attachComponent( particleSystemComponent );
}

ParticleSystem::~ParticleSystem( void )
{

}

const char *ParticleSystemComponent::NAME = "particleSystem";

ParticleSystemComponent::ParticleSystemComponent( void )
	: NodeComponent( NAME ),
	  _primitive( new Primitive( Primitive::Type::POINTS ) ),
	  _material( new Material() ),
 	  _particleCount( 50 ),
	  _particleSize( 20.0f ),
	  _particleDuration( 1.0f ),
	  _velocity( 1.0f, 1.0f, 1.0f ),
	  _gravity( 0.0f, -9.8f, 0.0f ),
	  _spread( 1.0f, 1.0f, 1.0f ),
	  _looping( true ),
	  _shape( nullptr ),
	  _gravityUniform( new Vector3fUniform( "uGravity", Vector3f( 0.0f, 0.0f, 0.0f ) ) ),
	  _timeUniform( new FloatUniform( "uTime", 0.0f ) ),
	  _durationUniform( new FloatUniform( "uLifeTime", 1.0f ) ),
	  _shapeRadiusUniform( new FloatUniform( "uShape.radius", 0.0f ) ),
	  _shapeCenterUniform( new Vector3fUniform( "uShape.center", Vector3f( 0.0f, 0.0f, 0.0f ) ) ),
	  _shapeFlagUniform( new BoolUniform( "uHasShape", false ) )
{
	ShaderProgramPtr program( new ParticleSystemShaderProgram() );

	program->attachUniform( _gravityUniform );
	program->attachUniform( _timeUniform );
	program->attachUniform( _durationUniform );
	program->attachUniform( _shapeRadiusUniform );
	program->attachUniform( _shapeCenterUniform );

	_material->setProgram( program );
	_material->getAlphaState()->setEnabled( true );
}

ParticleSystemComponent::~ParticleSystemComponent( void )
{

}

void ParticleSystemComponent::onAttach( void )
{
	Geometry *geometry = dynamic_cast< Geometry * >( getNode() );
	if ( geometry == nullptr ) {
		Log::Error << "Cannot attach a particle system to a node that is not a Geometry" << Log::End;
		exit( 1 );
	}

	generateParticles();
	geometry->attachPrimitive( _primitive );
	geometry->getComponent< MaterialComponent >()->attachMaterial( _material );
}

void ParticleSystemComponent::update( const Time &t )
{
	_durationUniform->setValue( _particleDuration );
	_gravityUniform->setValue( _gravity );
	_timeUniform->setValue( ( float ) 0.5f * t.getCurrentTime() );
	if ( _shape != nullptr ) {
		_shapeFlagUniform->setValue( true );
		_shapeRadiusUniform->setValue( _shape->getRadius() );
		_shapeCenterUniform->setValue( _shape->getCenter() );
	}
	else {
		_shapeFlagUniform->setValue( false );
	}
}

void ParticleSystemComponent::generateParticles( void )
{
	int particleCount = getParticleCount();

	VertexFormat format = VertexFormat::VF_P3_N3_UV2;
	VertexBufferObjectPtr vbo( new VertexBufferObject( format, particleCount, NULL ) );
	float *vertices = vbo->getData();

	IndexBufferObjectPtr ibo( new IndexBufferObject( particleCount, NULL ) );
	unsigned short *indices = ibo->getData();

	for ( unsigned short i = 0; i < particleCount; i++ ) {
		vertices[ i * format.getVertexSize() + 0 ] = getSpread()[ 0 ] * ( 2.0f * Numericf::random() - 1.0f );
		vertices[ i * format.getVertexSize() + 1 ] = getSpread()[ 1 ] * ( 2.0f * Numericf::random() - 1.0f );
		vertices[ i * format.getVertexSize() + 2 ] = getSpread()[ 2 ] * ( 2.0f * Numericf::random() - 1.0f );

		vertices[ i * format.getVertexSize() + 3 ] = getVelocity()[ 0 ];
		vertices[ i * format.getVertexSize() + 4 ] = getVelocity()[ 1 ];
		vertices[ i * format.getVertexSize() + 5 ] = getVelocity()[ 2 ];

		vertices[ i * format.getVertexSize() + 6 ] = getParticleSize();
		vertices[ i * format.getVertexSize() + 7 ] = Numericf::random( getParticleDuration() );

		indices[ i ] = i;
	}

	_primitive->setVertexBuffer( vbo );
	_primitive->setIndexBuffer( ibo );
}

int main( int argc, char **argv )
{
	SimulationPtr sim( new GLSimulation( "GPU Particle System", argc, argv ) );

	GroupPtr scene( new Group() );

	ParticleSystemPtr particleSystem( new ParticleSystem() );
	ParticleSystemComponent *particleSystemComponent = particleSystem->getComponent< ParticleSystemComponent >();
	particleSystemComponent->setParticleCount( 500 );
	particleSystemComponent->setParticleSize( 20.0f );
	particleSystemComponent->setParticleDuration( 1.0f );
	particleSystemComponent->setGravity( Vector3f( 0.0f, -9.8f, 0.0f ) );
	particleSystemComponent->setSpread( Vector3f( 2.0f, 1.0f, 2.0f ) );
	particleSystemComponent->setVelocity( Vector3f( 10.0f, 0.0f, 0.0f ) );
	particleSystemComponent->setLooping( true );
	particleSystemComponent->generateParticles();
	ImagePtr image( new ImageTGA( FileSystem::getInstance().pathForResource( "particle.tga" ) ) );
	TexturePtr texture( new Texture( image ) );
	particleSystemComponent->getParticleMaterial()->setColorMap( texture );
	scene->attachNode( particleSystem );

	PrimitivePtr ballPrimitive( new SpherePrimitive( 2.0f ) );
	GeometryPtr sphere( new Geometry() );
	sphere->attachPrimitive( ballPrimitive );
	MaterialPtr material( new Material() );
	material->setDiffuse( RGBAColorf( 1.0f, 0.0f, 0.0f, 1.0f ) );
	sphere->getComponent< MaterialComponent >()->attachMaterial( material );
	scene->attachNode( sphere );
	particleSystemComponent->setShape( sphere->worldBound() );

	LightPtr light( new Light() );
	light->local().setTranslate( 0.0f, 0.0f, 10.0f );
	scene->attachNode( light );

	CameraPtr camera( new Camera() );
	camera->local().setTranslate( 5.0f, -5.0f, 25.0f );
	scene->attachNode( camera );

	NodeComponentPtr controls( new LambdaComponent( [&]( Node *node, const Time &t ) {
		if ( InputState::getCurrentState().isKeyStillDown( 'W' ) ) {
			sphere->local().translate() += Vector3f( 0.0f, 4.0f * t.getDeltaTime(), 0.0f );
		}
		if ( InputState::getCurrentState().isKeyStillDown( 'S' ) ) {
			sphere->local().translate() += Vector3f( 0.0f, -4.0f * t.getDeltaTime(), 0.0f );
		}
		if ( InputState::getCurrentState().isKeyStillDown( 'A' ) ) {
			sphere->local().translate() += Vector3f( -4.0f * t.getDeltaTime(), 0.0f, 0.0f );
		}
		if ( InputState::getCurrentState().isKeyStillDown( 'D' ) ) {
			sphere->local().translate() += Vector3f( 4.0f * t.getDeltaTime(), 0.0f, 0.0f );
		}

		if ( InputState::getCurrentState().isKeyStillDown( 'Q' ) ) {
			particleSystem->local().rotate() *= Quaternion4f::createFromAxisAngle( Vector3f( 0.0f, 0.0f, 1.0f ), 0.1f );
		}
		if ( InputState::getCurrentState().isKeyStillDown( 'E' ) ) {
			particleSystem->local().rotate() *= Quaternion4f::createFromAxisAngle( Vector3f( 0.0f, 0.0f, 1.0f ), -0.1f );
		}
	}));
	scene->attachComponent( controls );

	sim->attachScene( scene );
	return sim->run();
}

