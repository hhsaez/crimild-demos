#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( location = 0 ) in vec3 inWorldPos;
layout ( location = 1 ) in vec3 inWorldNormal;
layout ( location = 2 ) in vec3 inWorldEye;

layout ( binding = 1 ) uniform samplerCube texSampler;

layout ( location = 0 ) out vec4 outColor;

void main()
{
	vec3 I = normalize( inWorldPos - inWorldEye );
	vec3 R = reflect( I, normalize( inWorldNormal ) );

	R.y *= -1.0;

	outColor = vec4( texture( texSampler, R ).rgb, 1.0 );
}

