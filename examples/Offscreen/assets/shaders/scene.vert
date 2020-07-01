#version 450

layout ( location = 0 ) in vec3 inPosition;
layout ( location = 1 ) in vec3 inColor;
layout ( location = 2 ) in vec2 inTextureCoord;

layout ( set = 0, binding = 0 ) uniform RenderPassUniforms {
    mat4 view;
	mat4 proj;
};

layout ( set = 1, binding = 0 ) uniform ModelUniforms {
	mat4 model;
};

layout ( location = 0 ) out vec3 outColor;
layout ( location = 1 ) out vec2 outTextureCoord;

void main()
{
	gl_Position = proj * view * model * vec4( inPosition, 1.0 );
	outColor = inColor;
	outTextureCoord = inTextureCoord;
}
