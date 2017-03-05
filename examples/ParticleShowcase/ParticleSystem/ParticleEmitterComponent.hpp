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

#ifndef CRIMILD_PARTICLE_EMITTER_COMPONENT_
#define CRIMILD_PARTICLE_EMITTER_COMPONENT_

#include "ParticleData.hpp"

namespace crimild {

    /**
        \brief Generates new particles for a particle system

		An emitter generates up to _emitRate particles per second		

		\warning Requires an instance of ParticleSystemComponent attached
		to the parent node
    */
    class ParticleEmitterComponent : public NodeComponent {
        CRIMILD_IMPLEMENT_RTTI( crimild::ParticleEmitterComponent )

    public:
		/**
		   \brief A generator for particle attributes
		 */
        class ParticleGenerator {
        public:
            virtual ~ParticleGenerator( void ) { }

			/**
			   \brief Configures the particle generator

			   This method is invoked in ParticleEmitterComponent::start()
			 */
			virtual void configure( ParticleData *particles ) = 0;

			/**
			   \brief Generates data for one or more attributes
			 */
            virtual void generate( crimild::Real64 dt, ParticleData *particles, ParticleId startId, ParticleId endId ) = 0;
        };

        using ParticleGeneratorPtr =  SharedPointer< ParticleGenerator >;

    public:
        ParticleEmitterComponent( void );
        virtual ~ParticleEmitterComponent( void );

		inline void setEmitRate( crimild::Size value ) { _emitRate = value; }
		inline crimild::Size getEmitRate( void ) const { return _emitRate; }

		inline void addGenerator( ParticleGeneratorPtr const &gen )
		{
			_generators.add( gen );
		}

		virtual void start( void ) override;

		/**
		   \brief Generates a fraction of _emitRate particles per frame,
		   provided there are enough particles available in the pool
		 */
        virtual void update( const Clock &c ) override;
        
    private:
        crimild::Size _emitRate;
        ThreadSafeArray< ParticleGeneratorPtr > _generators;
		ParticleData *_particles = nullptr;
    };

}

#endif

