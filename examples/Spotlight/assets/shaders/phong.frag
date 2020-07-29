#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( location = 0 ) in vec3 inWorldPosition;
layout ( location = 1 ) in vec3 inWorldNormal;
layout ( location = 2 ) in vec3 inWorldEye;
layout ( location = 3 ) in vec2 inTexCoord;

struct LightData {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 position;
	vec4 direction;
	vec4 attenuation;
	vec4 cutoffs;
};

layout ( binding = 1 ) uniform LightingUniform {
	LightData light[ 10 ];
    int pointLightCount;
};

layout ( location = 0 ) out vec4 outColor;

float calcAttenuation( float D, vec3 att )
{
	float C = att.x;
	float X = att.y;
	float X2 = att.z;
	return 1.0 / ( C + ( X * D ) + X2 * D * D );
}

vec3 calcPhongSpotlight( LightData light, vec3 P, vec3 N, vec3 E )
{
	vec3 lAmbient = light.ambient.rgb;
	vec3 lDiffuse = light.diffuse.rgb;
	vec3 lSpecular = light.specular.rgb;
	vec3 lPosition = light.position.xyz;
	vec3 lDirection = light.direction.xyz;
	vec3 lAttenuation = light.attenuation.xyz;
	float lInnerCutoff = light.cutoffs.x;
	float lOuterCutoff = light.cutoffs.y;

	vec3 L = lPosition - P;
	float lDistance = length( L );
	L = normalize( L );
	vec3 H = normalize( L + E );

	float att = calcAttenuation( lDistance, lAttenuation );
	float dNL = dot( N, L );

	vec3 ambient = att * lAmbient;
	vec3 diffuse = att * lDiffuse * max( 0.0, dNL );
	vec3 specular = att * lSpecular * pow( max( dot( N, H ), 0.0 ), 32 );

	float theta = dot( L, normalize( -lDirection ) );
	float epsilon = lInnerCutoff - lOuterCutoff;
	float intensity = clamp( ( theta - lOuterCutoff ) / epsilon, 0.0, 1.0 );
	diffuse *= intensity;
	specular *= intensity;

	return ambient + diffuse + specular;
}

void main()
{
	vec3 albedo = vec4( 1.0 ).rgb;

	vec3 P = inWorldPosition;
	vec3 N = normalize( inWorldNormal );
	vec3 E = inWorldEye;

	vec3 accum = vec3( 0 );
	accum += calcPhongSpotlight( light, P, N, E );

	outColor = vec4( albedo * accum, 1.0 );
}
