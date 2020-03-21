#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( location = 0 ) in vec2 inTexCoord;

layout ( binding = 0 ) uniform Context {
    vec4 dimensions;
} context;

layout ( location = 0 ) out vec4 outColor;

float circleMask( vec2 uv, vec2 p, float r, float blur )
{
	float d = length( uv - p );
	float c = smoothstep( r, r - blur, d );
	return c;
}

void main()
{
	vec2 uv = inTexCoord;
	uv -= 0.5;
	uv.x *= context.dimensions.x / context.dimensions.y;

	float blur = 0.00625;

	float mask = circleMask( uv, vec2( 0.0 ), 0.4, blur );
	mask -= circleMask( uv, vec2( -0.15, 0.1 ), 0.075, blur );
	mask -= circleMask( uv, vec2( 0.15, 0.1 ), 0.075, blur );
	vec3 faceColor = vec3( 1.0, 1.0, 0.0 ) * mask;

	mask = circleMask( uv, vec2( 0.0 ), 0.25, blur );
	mask -= circleMask( uv, vec2( 0.0, 0.05 ), 0.25, blur );
	mask *= uv.y <= 0.0 ? 1.0 : 0.0;
	vec3 mouthColor = vec3( 1.0 ) * mask;

	vec3 color = faceColor - mouthColor;

	outColor = vec4( color, 1.0 );
}

