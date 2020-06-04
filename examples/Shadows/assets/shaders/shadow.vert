#version 450

layout ( location = 0 ) in vec3 inPosition;
layout ( location = 1 ) in vec3 inNormal;
layout ( location = 2 ) in vec2 inTextureCoord;

layout ( set = 0, binding = 0 ) uniform Uniforms {
	mat4 lightSpaceMatrix;
};

layout ( set = 1, binding = 0 ) uniform ModelUniforms {
	mat4 model;
};

void main()
{
	gl_Position = lightSpaceMatrix * model * vec4( inPosition, 1.0 );
}

