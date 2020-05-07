#version 450

layout ( location = 0 ) in vec3 inPosition;
layout ( location = 1 ) in vec3 inColor;
layout ( location = 2 ) in vec2 inTextureCoord;

layout ( binding = 0 ) uniform MVP {
	mat4 model;
    mat4 view;
	mat4 proj;
};

layout ( location = 0 ) out vec3 outColor;
layout ( location = 1 ) out vec2 outTextureCoord;
layout ( location = 2 ) out vec4 outPosition;

void main()
{
	gl_Position = proj * view * model * vec4( inPosition, 1.0 );
	outColor = inColor;
	outTextureCoord = inTextureCoord;
	outPosition = gl_Position;
}

