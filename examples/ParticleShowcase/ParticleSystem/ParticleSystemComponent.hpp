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

#ifndef CRIMILD_PARTICLE_SYSTEM_COMPONENT_
#define CRIMILD_PARTICLE_SYSTEM_COMPONENT_

#include "ParticleData.hpp"

namespace crimild {

    /**
        \remarks Since it's a component, the particle sytem
        can affect the parent node any wait it likes

        For exmaple, it might add a geometry for simple particles

        In addition, if the particles are actual nodes, the "renderer"
        will affect the transform for each node

        MUST BE ATTACHED TO A GROUP NODE (or derived class)
    */
    class ParticleSystemComponent : public NodeComponent {
        CRIMILD_IMPLEMENT_RTTI( crimild::ParticleSystemComponent )

    public:
		/**
		   \brief Deafult constructor
		 */
        ParticleSystemComponent( ParticleDataPtr const &particles );
        virtual ~ParticleSystemComponent( void );

        inline ParticleData *getParticles( void ) { return crimild::get_ptr( _particles ); }

		virtual void onAttach( void ) override;
		virtual void onDetach( void ) override;

    private:
        SharedPointer< ParticleData > _particles;
    };

}

#endif

