#version 450

layout ( location = 0 ) in vec3 inPosition;
layout ( location = 1 ) in vec3 inNormal;
layout ( location = 2 ) in vec2 inTextureCoord;

layout ( set = 0, binding = 0 ) uniform RenderPassUniforms {
    mat4 view;
	mat4 proj;
	mat4 lightSpace;
	vec3 lightPos;
};

layout ( set = 1, binding = 0 ) uniform ModelUniforms {
	mat4 model;
};

layout ( location = 0 ) out vec3 outNormal;
layout ( location = 1 ) out vec2 outTextureCoord;
layout ( location = 2 ) out vec3 outLightVec;
layout ( location = 3 ) out vec3 outViewVec;
layout ( location = 4 ) out vec4 outShadowCoord;

const mat4 bias = mat4(
    0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0
);

void main()
{
	vec4 pos = model * vec4( inPosition, 1.0 );
	gl_Position = proj * view * pos;

	outNormal = mat3( model ) * inNormal;
	outTextureCoord = inTextureCoord;
	outLightVec = normalize( lightPos - pos.xyz );
	outViewVec = -pos.xyz;
	outShadowCoord = bias * lightSpace * pos;
}

