#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( location = 0 ) in vec3 inColor;
layout ( location = 1 ) in vec2 inTextureCoord;

layout ( binding = 2 ) uniform sampler2D uSampler;

layout ( location = 0 ) out vec4 outColor;

void main()
{
	outColor = vec4( inColor * texture( uSampler, inTextureCoord ).rgb, 1.0 );
}

