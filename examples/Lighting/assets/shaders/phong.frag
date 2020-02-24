#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( location = 0 ) in vec3 inWorldPosition;
layout ( location = 1 ) in vec3 inWorldNormal;
layout ( location = 2 ) in vec3 inWorldEye;
layout ( location = 3 ) in vec2 inTexCoord;

layout ( binding = 1 ) uniform sampler2D colorMap;

struct LightData {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 position;
};

layout ( binding = 2 ) uniform LightDataUniform {
	LightData lights[ 3 ];
};

layout ( location = 0 ) out vec4 outColor;

vec3 applyLight( LightData light, vec3 P, vec3 N, vec3 V )
{
	vec3 ambient = light.ambient.rgb;

	vec3 Lp = light.position.xyz;
	vec3 Ld = normalize( Lp - P );

	float L = max( 0, dot( N, Ld ) );
	vec3 diffuse = L * light.diffuse.rgb;

	vec3 R = reflect( -Ld, N );
	float S = pow( max( dot( V, R ), 0.0 ), 32 );
	vec3 specular = S * light.specular.rgb;

	return ambient + diffuse + specular;
}

void main()
{
	vec3 albedo = texture( colorMap, inTexCoord ).rgb;

	vec3 ambient = lights[ 0 ].ambient.rgb;

	vec3 P = inWorldPosition;
	vec3 N = normalize( inWorldNormal );
	vec3 E = inWorldEye;
	vec3 V = normalize( E - P );

	vec3 accum = vec3( 0 );
	accum += applyLight( lights[ 0 ], P, N, V );
	accum += applyLight( lights[ 1 ], P, N, V );
	accum += applyLight( lights[ 2 ], P, N, V );

	outColor = vec4( albedo * accum, 1.0 );
}

