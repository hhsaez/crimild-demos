#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( location = 0 ) in vec3 inNormal;
layout ( location = 1 ) in vec3 inPosition;
layout ( location = 2 ) in vec3 inEyePosition;
layout ( location = 3 ) in vec3 inLightDirection;

layout ( location = 0 ) out vec4 outColor;

vec3 ambient()
{
    return vec3( 0.1 );
}

vec3 diffuse( vec3 N, vec3 L )
{
    float d = max( dot( N, L ), 0.0 );
    return vec3( d );
}

vec3 specular( vec3 N, vec3 E, vec3 P )
{
    float strength = 0.5;
    vec3 V = normalize( E - P );
    vec3 R = reflect( -V, N );
    float s = pow( max( dot( V, R ), 0.0 ), 32 );
    return vec3( strength * s );
}

void main()
{
    vec3 N = normalize( inNormal );
    vec3 P = inPosition;
    vec3 E = inEyePosition;
    vec3 L = normalize( inLightDirection );

    vec3 lightColor = vec3( 1.0, 0.0, 0.0 );

	outColor = vec4( lightColor * ( ambient() + diffuse( N, L ) + specular( N, E, P ) ), 1.0 );
}
