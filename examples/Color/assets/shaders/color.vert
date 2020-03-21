#version 450

layout ( location = 0 ) in vec3 inPosition;

layout ( binding = 0 ) uniform TransformBuffer {
	mat4 model;
	mat4 view;
	mat4 proj;
};

void main()
{
	gl_Position = proj * view * model * vec4( inPosition, 1.0 );
}

