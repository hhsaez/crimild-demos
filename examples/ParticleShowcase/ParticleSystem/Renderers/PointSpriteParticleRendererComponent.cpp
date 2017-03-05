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

#include "PointSpriteParticleRendererComponent.hpp"

#include "../ParticleSystemComponent.hpp"

using namespace crimild;

PointSpriteParticleRendererComponent::PointSpriteParticleRendererComponent( void )
{
	// create the material here so it can be modified later
	_material = crimild::alloc< Material >();

    auto program = crimild::retain( AssetManager::getInstance()->get< ShaderProgram >( Renderer::SHADER_PROGRAM_POINT_SPRITE ) );
    _material->setProgram( program );
}

PointSpriteParticleRendererComponent::~PointSpriteParticleRendererComponent( void )
{

}

void PointSpriteParticleRendererComponent::onAttach( void )
{
	_geometry = crimild::alloc< Geometry >();
	if ( _material != nullptr ) {
		_geometry->getComponent< MaterialComponent >()->attachMaterial( _material );
	}

	getNode< Group >()->attachNode( _geometry );
}

void PointSpriteParticleRendererComponent::onDetach( void )
{

}

void PointSpriteParticleRendererComponent::start( void )
{
	const auto ps = getComponent< ParticleSystemComponent >();
	_particles = ps->getParticles();
	_positions = _particles->getAttrib( ParticleAttribType::POSITION )->getData< Vector3f >();
	_colors = _particles->getAttrib( ParticleAttribType::COLOR )->getData< RGBAColorf >();
	_sizes = _particles->getAttrib( ParticleAttribType::UNIFORM_SCALE )->getData< crimild::Real32 >();

    _primitive = crimild::alloc< Primitive >( Primitive::Type::POINTS );

	_geometry->attachPrimitive( _primitive );
}

void PointSpriteParticleRendererComponent::update( const Clock &c )
{
	// TODO: sort particles back-to-front?

    const auto pCount = _particles->getAliveCount();
    if ( pCount == 0 ) {
        return;
    }
    
    auto vbo = crimild::alloc< VertexBufferObject >( VertexFormat::VF_P3_C4_UV2, pCount );
    
    // traversing the arrays in separated loops seems to be more cache-friendly, right? right?
    
	for ( auto i = 0; i < pCount; i++ ) {
		vbo->setPositionAt( i, _positions[ i ] );
	}

	for ( auto i = 0; i < pCount; i++ ) {
		vbo->setTextureCoordAt( i, Vector2f( _sizes[ i ], 0.0f ) );
	}

	for ( auto i = 0; i < pCount; i++ ) {
		vbo->setRGBAColorAt( i, _colors[ i ] );
	}

    auto ibo = crimild::alloc< IndexBufferObject >( pCount );
    ibo->generateIncrementalIndices();
    
    crimild::concurrency::sync_frame( [this, vbo, ibo] {
        _primitive->setVertexBuffer( vbo );
        _primitive->setIndexBuffer( ibo );
    });
}


