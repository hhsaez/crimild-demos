#version 450

layout ( location = 0 ) in vec3 inPosition;
layout ( location = 1 ) in vec3 inNormal;

layout ( set = 0, binding = 0 ) uniform RenderPassUniforms {
    mat4 view;
	mat4 proj;
};

layout ( set = 0, binding = 1 ) uniform LightingUniforms {
    vec4 lightDirection;
};

layout ( set = 1, binding = 0 ) uniform GeometryUniforms {
	mat4 model;
};

layout ( location = 0 ) out vec3 outNormal;
layout ( location = 1 ) out vec3 outPosition;
layout ( location = 2 ) out vec3 outEyePosition;
layout ( location = 3 ) out vec3 outLightDirection;

void main()
{
	gl_Position = proj * view * model * vec4( inPosition, 1.0 );

    outNormal = normalize( mat3( transpose( inverse( model ) ) ) * inNormal );
    outPosition = vec3( model * vec4( inPosition, 1.0 ) );
    outLightDirection = normalize( -lightDirection.xyz );
    outEyePosition = vec3( inverse( view )[ 3 ] );
}
