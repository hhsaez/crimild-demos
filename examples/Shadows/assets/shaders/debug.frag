#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( location = 0 ) in vec2 inTextureCoord;

layout ( set = 0, binding = 0 ) uniform DebugUniforms {
	int attachmentType;
};

layout ( set = 0, binding = 1 ) uniform sampler2D uColorMap;

layout ( location = 0 ) out vec4 outColor;

float linearizeDepth( float depth )
{
	float n = 1.0;
	float f = 128.0;
	float z = depth;
	return ( 2.0 * n ) / ( f + n - z * ( f - n ) ); 
}

void main()
{
	vec3 color = texture( uColorMap, inTextureCoord ).rgb;

	if ( attachmentType == 1 ) {
		// attachment is depth
		color = vec3( linearizeDepth( color.r ) );
	}

	outColor = vec4( color, 1.0 );
}

