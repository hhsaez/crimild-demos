#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( location = 0 ) in vec3 inPosition;
layout ( location = 1 ) in vec3 inNormal;
layout ( location = 2 ) in vec3 inEye;

layout ( binding = 1 ) uniform Material {
    vec4 color;
} material;

layout ( binding = 2 ) uniform Light {
	vec4 color;
	vec4 position;
} light;

layout ( location = 0 ) out vec4 outColor;

void main()
{
	vec3 P = inPosition;
	vec3 N = normalize( inNormal );
	vec3 mColor = material.color.rgb;
	vec3 lDirection = normalize( light.position.xyz - P );
	vec3 lColor = light.color.rgb;
	vec3 vDirection = normalize( inEye - inPosition );
	vec3 halfVector = normalize( lDirection + vDirection );

	float ambientStrength = 0.1;
	vec3 ambient = ambientStrength * lColor;

	float L = max( dot( N, lDirection ), 0.0 );
	vec3 diffuse = L * lColor;

	float specularStrength = 0.5;
	float S = pow( max( dot( N, halfVector ), 0.0 ), 32.0 );
	vec3 specular = specularStrength * S * lColor;

	vec3 result = ( ambient + diffuse + specular ) * mColor;

	outColor = vec4( result.rgb, 1.0 );
}

