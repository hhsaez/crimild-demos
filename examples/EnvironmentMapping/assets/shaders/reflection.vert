#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( location = 0 ) in vec3 inPosition;
layout ( location = 1 ) in vec3 inNormal;
layout ( location = 2 ) in vec2 inTexCoord;

layout ( binding = 0 ) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout ( location = 0 ) out vec3 outWorldPos;
layout ( location = 1 ) out vec3 outWorldNormal;
layout ( location = 2 ) out vec3 outWorldEye;

void main()
{
	vec4 worldPos = ubo.model * vec4( inPosition, 1.0 );
	gl_Position = ubo.proj * ubo.view * worldPos;
	outWorldPos = worldPos.xyz;
	
	outWorldNormal = transpose( inverse( mat3( ubo.model ) ) ) * inNormal;
	outWorldNormal.y *= -1.0;

	mat4 invView = inverse( ubo.view );
	outWorldEye = vec3( invView[ 3 ].x, invView[ 3 ].y, invView[ 3 ].z );
}

