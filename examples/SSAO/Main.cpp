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

Node *makeBall( float x, float y, float z, float radius )
{
    Geometry *ball = new Geometry( "ball" );
    ball->attachPrimitive( new SpherePrimitive( radius, VertexFormat::VF_P3_N3 ) );
    ball->local().setTranslate( x, y, z );
    return ball;
}

int main( int argc, char **argv )
{
	GLSimulation sim( "Crimild Demo: Screen Space Ambient Occlusion", argc, argv );

	Pointer< Group > scene( new Group() );
    
    Group *balls = new Group();
    for ( int line = 0; line < 5; line++ ) {
        for ( float x = 0; x < line; x++ ) {
            for ( float z = 0; z < line; z++ ) {
                balls->attachNode( makeBall( 2 * ( x - 0.5f * line ), 1.65f * ( 3 - line ), 2 * ( z - 0.5f * line ), 1.0f ) );
            }
        }
    }
    balls->attachComponent( new RotationComponent( Vector3f( 0.0f, 1.0f, 0.0f ), 0.01f ) );
    scene->attachNode( balls );

	Pointer< Camera > camera( new Camera() );
    camera->setRenderPass( new DeferredRenderPass() );
	camera->local().setTranslate( 0.0f, 0.0f, 10.0f );
    camera->local().lookAt( Vector3f( 0.0f, 0.0f, 0.0f ) );
	scene->attachNode( camera.get() );
    
    Light *light = new Light();
    light->local().setTranslate( 0.0f, 5.0f, 5.0f );
    scene->attachNode( light );

	sim.setScene( scene.get() );
	return sim.run();
}

