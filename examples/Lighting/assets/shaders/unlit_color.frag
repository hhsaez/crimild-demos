#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( binding = 1 ) uniform Material {
	vec4 albedo;
} material;

layout ( location = 0 ) out vec4 outColor;

void main()
{
	outColor = vec4( material.albedo.rgb, 1.0 );
}

