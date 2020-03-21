#version 450

layout ( location = 0 ) in vec3 inPosition;
layout ( location = 1 ) in vec3 inNormal;

layout ( binding = 0 ) uniform TransformBuffer {
	mat4 model;
	mat4 view;
	mat4 proj;
};

layout ( location = 0 ) out vec3 outPosition;
layout ( location = 1 ) out vec3 outNormal;
layout ( location = 2 ) out vec3 outEye;

void main()
{
	mat4 invView = inverse( view );
	
	outPosition = vec3( model * vec4( inPosition, 1.0 ) );
	outNormal = normalize( mat3( transpose( inverse( model ) ) ) * inNormal );
	outEye = invView[ 3 ].xyz;
	
	gl_Position = proj * view * vec4( outPosition, 1.0 );

}

