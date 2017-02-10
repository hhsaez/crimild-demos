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

SharedPointer< Node > createTexturedQuad( const Vector3f &position, Texture::Filter minFilter, Texture::Filter magFilter )
{
    auto quad = crimild::alloc< Geometry >();
    quad->attachPrimitive( crimild::alloc< QuadPrimitive >( 1.9f, 1.9f, VertexFormat::VF_P3_UV2 ) );
    quad->local().setTranslate( position );

    auto material = crimild::alloc< Material >();
    material->setProgram( Renderer::getInstance()->getShaderProgram( Renderer::SHADER_PROGRAM_UNLIT_TEXTURE ) );
    auto texture = crimild::alloc< Texture >( crimild::alloc< ImageTGA >( FileSystem::getInstance().pathForResource( "noise.tga" ) ) );
    texture->setMinFilter( minFilter );
    texture->setMagFilter( magFilter );
    material->setColorMap( texture );    
    quad->getComponent< MaterialComponent >()->attachMaterial( material );

    return quad;
}

int main( int argc, char **argv )
{
    auto sim = crimild::alloc< GLSimulation >( "Texture Filters", crimild::alloc< Settings >( argc, argv ) );

    auto scene = crimild::alloc< Group >();
    scene->attachNode( createTexturedQuad( Vector3f( -1.0f, +1.0f, 0.0f ), Texture::Filter::LINEAR, Texture::Filter::LINEAR ) );
    scene->attachNode( createTexturedQuad( Vector3f( -1.0f, -1.0f, 0.0f ), Texture::Filter::LINEAR, Texture::Filter::NEAREST ) );
    scene->attachNode( createTexturedQuad( Vector3f( +1.0f, +1.0f, 0.0f ), Texture::Filter::NEAREST, Texture::Filter::LINEAR ) );
    scene->attachNode( createTexturedQuad( Vector3f( +1.0f, -1.0f, 0.0f ), Texture::Filter::NEAREST, Texture::Filter::NEAREST ) );

    auto camera = crimild::alloc< Camera >();
    camera->local().setTranslate( Vector3f( 0.0f, 0.0f, 3.0f ) );
    scene->attachNode( camera );
    
    sim->setScene( scene );
	return sim->run();
}

