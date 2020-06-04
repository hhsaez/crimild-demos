#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( set = 1, binding = 1 ) uniform MaterialUniform {
    vec4 uColor;
};

layout ( set = 1, binding = 2 ) uniform sampler2D uColorMap;

layout ( location = 0 ) out vec4 outColor;

void main()
{
	outColor = vec4( 1.0, 1.0, 0.0, 1.0 );
}

