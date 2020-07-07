#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( location = 0 ) in vec3 inPosition;
layout ( location = 1 ) in vec3 inNormal;

layout ( set = 0, binding = 0 ) uniform RenderPassUniforms {
    mat4 view;
	mat4 proj;
};

layout ( set = 2, binding = 0 ) uniform GeometryUniforms {
	mat4 model;
};

layout ( location = 0 ) out vec3 outWorldPos;
layout ( location = 1 ) out vec3 outWorldNormal;
layout ( location = 2 ) out vec3 outWorldEye;

void main()
{
	vec4 worldPos = model * vec4( inPosition, 1.0 );
	gl_Position = proj * view * worldPos;
	outWorldPos = worldPos.xyz;

	outWorldNormal = normalize( mat3( transpose( inverse( model ) ) ) * inNormal );

	mat4 invView = inverse( view );
	outWorldEye = vec3( invView[ 3 ].x, invView[ 3 ].y, invView[ 3 ].z );
}
