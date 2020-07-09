#version 450

layout ( location = 0 ) in vec3 inPosition;
layout ( location = 1 ) in vec3 inNormal;

layout ( set = 0, binding = 0 ) uniform RenderPassUniforms {
    mat4 view;
	mat4 proj;
};

layout ( set = 0, binding = 1 ) uniform LightingUniforms {
    vec4 lightDirection;
};

layout ( set = 1, binding = 0 ) uniform GeometryUniforms {
	mat4 model;
};

layout ( location = 0 ) out vec3 outColor;

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
    vec3 N = normalize( mat3( transpose( inverse( model ) ) ) * inNormal );
    vec3 P = vec3( model * vec4( inPosition, 1.0 ) );
    vec3 L = normalize( -lightDirection.xyz );
    vec3 E = vec3( inverse( view )[ 3 ] );

    vec3 lightColor = vec3( 1.0, 0.0, 0.0 );

	gl_Position = proj * view * model * vec4( inPosition, 1.0 );
    outColor = lightColor * ( ambient() + diffuse( N, L ) + specular( N, E, P ) );
}
