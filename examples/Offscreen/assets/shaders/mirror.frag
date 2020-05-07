#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( location = 0 ) in vec3 inColor;
layout ( location = 1 ) in vec2 inTextureCoord;
layout ( location = 2 ) in vec4 inPosition;

layout ( binding = 1 ) uniform sampler2D uSampler;

layout ( location = 0 ) out vec4 outColor;

void main()
{
	vec4 tmp = vec4( 1.0 / inPosition.w );
	vec4 ndcCoord = inPosition * tmp;
	ndcCoord += vec4( 1.0 );
	ndcCoord *= vec4( 0.5 );

	const float BLUR_SIZE = 1.0 / 512.0;

	vec2 uv = ndcCoord.xy;
	uv -= 0.5;
	float d = length( uv );
	float c = smoothstep( 0.4, 0.2, d );

	vec3 color = vec3( 0.0 );
	if ( gl_FrontFacing ) {
	   vec3 reflection = vec3( 0.0f );
	   for ( int y = -3; y <= 3; y++ ) {
	   	   for ( int x = -1; x <= 3; x++ ) {
		   	   reflection += texture( uSampler, vec2( ndcCoord.s + x * BLUR_SIZE, 1.0 - ( ndcCoord.t + y * BLUR_SIZE ) ) ).rgb / 49.0;
		   }
	   }
	   color += reflection;
	}

	outColor = vec4( color * c, 1.0 );
}

