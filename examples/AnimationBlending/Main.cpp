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
using namespace crimild::animation;

using UpdateCallback = LambdaComponent;

SharedPointer< Node > createTriangle( void )
{
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
    material->setProgram( AssetManager::getInstance()->get< ShaderProgram >( Renderer::SHADER_PROGRAM_UNLIT_VERTEX_COLOR ) );
    material->getCullFaceState()->setEnabled( false );
    geometry->getComponent< MaterialComponent >()->attachMaterial( material );

	auto centerAnim = crimild::alloc< Animation >(
		crimild::alloc< Clip >(
			"center",
			crimild::alloc< Quaternion4fChannel >(
				"rotation",
				containers::Array< crimild::Real32 > { 2.0 },
				containers::Array< Quaternion4f > { Quaternion4f::createFromAxisAngle( Vector3f::UNIT_Y, 0.0 ) }
			)
		)
	);

	auto leftAnim = crimild::alloc< Animation >(
		crimild::alloc< Clip >(
			"left",
			crimild::alloc< Quaternion4fChannel >(
				"rotation",
				containers::Array< crimild::Real32 > { 2.0 },
				containers::Array< Quaternion4f > { Quaternion4f::createFromAxisAngle( Vector3f::UNIT_Y, -0.45 * Numericf::PI ) }
			)
		)
	);

	auto rightAnim = crimild::alloc< Animation >(
		crimild::alloc< Clip >(
			"right",
			crimild::alloc< Quaternion4fChannel >(
				"rotation",
				containers::Array< crimild::Real32 > { 2.0 },
				containers::Array< Quaternion4f > { Quaternion4f::createFromAxisAngle( Vector3f::UNIT_Y, 0.45 * Numericf::PI ) }
			)
		)
	);

	auto upAnim = crimild::alloc< Animation >(
		crimild::alloc< Clip >(
			"up",
			crimild::alloc< Quaternion4fChannel >(
				"rotation",
				containers::Array< crimild::Real32 > { 2.0 },
				containers::Array< Quaternion4f > { Quaternion4f::createFromAxisAngle( Vector3f::UNIT_X, -0.45 * Numericf::PI ) }
			)
		)
	);

	auto downAnim = crimild::alloc< Animation >(
		crimild::alloc< Clip >(
			"down",
			crimild::alloc< Quaternion4fChannel >(
				"rotation",
				containers::Array< crimild::Real32 > { 2.0 },
				containers::Array< Quaternion4f > { Quaternion4f::createFromAxisAngle( Vector3f::UNIT_X, 0.45 * Numericf::PI ) }
			)
		)
	);

	geometry->attachComponent< UpdateCallback >( [ centerAnim, leftAnim, rightAnim, upAnim, downAnim ]( Node *node, const Clock &c ) {

		auto mousePos = Input::getInstance()->getNormalizedMousePosition();

		auto xLeft = Numericf::clamp( 2.0f * mousePos.x(), 0.0f, 1.0f );
		auto xRight = Numericf::clamp( 2.0f * ( mousePos.x() - 0.5f ), 0.0f, 1.0f );
		leftAnim->update( c )->lerp( centerAnim, xLeft )->lerp( rightAnim, xRight );

		auto yUp = Numericf::clamp( 2.0f * mousePos.y(), 0.0f, 1.0f );
		auto yDown = Numericf::clamp( 2.0f * ( mousePos.y() - 0.5f ), 0.0f, 1.0f );
		upAnim->update( c )->lerp( centerAnim, yUp )->lerp( downAnim, yDown );

		leftAnim->lerp( upAnim, 0.5f, false )->getValue( "rotation", node->local().rotate() );

	});

	return geometry;
}

int main( int argc, char **argv )
{
    auto sim = crimild::alloc< GLSimulation >( "Animation Blending", crimild::alloc< Settings >( argc, argv ) );

    auto scene = crimild::alloc< Group >();

	scene->attachNode( createTriangle() );

    auto camera = crimild::alloc< Camera >();
    camera->local().setTranslate( Vector3f( 0.0f, 0.0f, 3.0f ) );
    scene->attachNode( camera );
    
    sim->setScene( scene );
	return sim->run();
}
