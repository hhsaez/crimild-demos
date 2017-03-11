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

#ifndef CRIMILD_PARTICLE_GENERATOR_RANDOM_
#define CRIMILD_PARTICLE_GENERATOR_RANDOM_

#include "../ParticleSystemComponent.hpp"

namespace crimild {

	template< typename T >
    class RandomAttribParticleGenerator : public ParticleSystemComponent::ParticleGenerator {
    public:
        RandomAttribParticleGenerator( void )
		{

		}
		
        virtual ~RandomAttribParticleGenerator( void )
		{

		}

		inline void setParticleAttribType( const ParticleAttribType &type ) { _attribType = type; }
		inline const ParticleAttribType &ParticleAttribType( void ) const { return _attribType; }

        inline void setMinValue( const Vector3f &value ) { _minValue = value; }
        inline const Vector3f &getMinValue( void ) const { return _minValue; }

        inline void setMaxValue( const Vector3f &value ) { _maxValue = value; }
        inline const Vector3f &getMaxValue( void ) const { return _maxValue; }

		virtual void configure( Node *node, ParticleData *particles ) override
		{
			auto attribs = particles->getAttrib( _attribType );
			if ( attribs == nullptr ) {
				particles->setAttribs( _attribType, crimild::alloc< template ParticleAttribArray< T >>() );
				attribs = particles->getAttrib( _attribType );
			}
			_attribData = attribs->template getData< T >();
		}

        virtual void generate( Node *node, crimild::Real32 dt, ParticleData *particles, ParticleId startId, ParticleId endId ) override
		{
			for ( ParticleId i = startId; i < endId; i++ ) {
				_attribData[ i ] = template RandomGenerate< T >( _minValue, _maxValue );
			}
		}

    private:
		ParticleAttribType _attribType;
        T _minValue;
        T _maxValue;

		Vector3f *_attribData = nullptr;
    };

	using RandomVector3fParticleGenerator = RandomAttribParticleGenerator< Vector3f >;
	using RandomReal32ParticleGenerator = RandomAttribParticleGenerator< crimild::Real32 >();

}

#endif

