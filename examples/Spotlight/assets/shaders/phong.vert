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
layout ( location = 3 ) out vec2 outTexCoord;

void main()
{
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4( inPosition, 1.0 );

	outWorldPos = ( ubo.model * vec4( inPosition, 1.0 ) ).xyz;

	mat4 invView = inverse( ubo.view );
	outWorldEye = vec3( invView[ 3 ].x, invView[ 3 ].y, invView[ 3 ].z );
	
	outWorldNormal = normalize( mat3( transpose( inverse( ubo.model ) ) ) * inNormal );

	outTexCoord = inTexCoord;
}

