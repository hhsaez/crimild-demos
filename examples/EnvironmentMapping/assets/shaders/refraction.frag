#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( location = 0 ) in vec3 inWorldPos;
layout ( location = 1 ) in vec3 inWorldNormal;
layout ( location = 2 ) in vec3 inWorldEye;

layout ( binding = 1 ) uniform samplerCube texSampler;

layout ( location = 0 ) out vec4 outColor;

void main()
{
	float ratio = 1.0 / 1.33;
	vec3 I = normalize( inWorldPos - inWorldEye );
	vec3 R = refract( I, normalize( inWorldNormal ), ratio );

	// Because of Vulkan
	R.y *= -1.0;
	
	outColor = vec4( texture( texSampler, R ).rgb, 1.0 );
}

