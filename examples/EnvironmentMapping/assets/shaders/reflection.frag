#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( location = 0 ) in vec3 inWorldPos;
layout ( location = 1 ) in vec3 inWorldNormal;
layout ( location = 2 ) in vec3 inWorldEye;

layout ( set = 1, binding = 0 ) uniform samplerCube uSampler;

layout ( location = 0 ) out vec4 outColor;

vec3 reflected(vec3 I)
{
	vec3 R = reflect( I, normalize( inWorldNormal ) );
	return texture( uSampler, R ).rgb;
}

void main()
{
	vec3 I = normalize( inWorldPos - inWorldEye );
    outColor = vec4( reflected( I ), 1.0 );
}
