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

using namespace crimild;

int main( int argc, char **argv )
{
    auto sim = crimild::alloc< GLSimulation >( "Triangle", crimild::alloc< Settings >( argc, argv ) );

    auto scene = crimild::alloc< Group >();

    auto ps = crimild::alloc< ParticleSystem >();
    ps->setMaxParticles( 200 );
    ps->setParticleLifetime( 1.5f );
    ps->setParticleSpeed( 1.0f );
    ps->setParticleStartSize( 2.5f );
    ps->setParticleEndSize( 0.5f );
    ps->setParticleStartColor( RGBAColorf( 1.0f, 1.0f, 1.0f, 0.9f ) );
    ps->setParticleEndColor( RGBAColorf( 1.0f, 0.9f, 0.0f, 0.25f ) );
#if 0
    auto psEmitter = crimild::alloc< ConeParticleEmitter >( 1.0f, 0.25f );
    Transformation t;
    t.rotate().fromAxisAngle( Vector3f( 1.0f, 0.0f, 0.0f ), Numericf::PI );
    psEmitter->setTransformation( t );
#else
    auto psEmitter = crimild::alloc< CylinderParticleEmitter >( 0.1f, 0.5f );
    // auto psEmitter = crimild::alloc< SphereParticleEmitter >( 1.0f );
#endif
    ps->setEmitter( psEmitter );
    ps->setPreComputeParticles( true );
    ps->setTexture( crimild::retain( AssetManager::getInstance()->get< Texture >( "fire.tga" ) ) );
    ps->generate();

    auto g = crimild::alloc< Group >();
    g->attachNode( ps );
    g->local().setTranslate( 0.0f, -1.0f, 0.0f );
    scene->attachNode( g );

    auto camera = crimild::alloc< Camera >();
    camera->local().setTranslate( Vector3f( 0.0f, 0.0f, 5.0f ) );
    camera->setRenderPass( crimild::alloc< BasicRenderPass >() );
    scene->attachNode( camera );
    
    sim->setScene( scene );
	return sim->run();
}

