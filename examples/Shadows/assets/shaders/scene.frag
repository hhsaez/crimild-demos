#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( location = 0 ) in vec3 inNormal;
layout ( location = 1 ) in vec2 inTextureCoord;
layout ( location = 2 ) in vec3 inLightVec;
layout ( location = 3 ) in vec3 inViewVec;
layout ( location = 4 ) in vec4 inShadowCoord;

layout ( set = 0, binding = 1 ) uniform sampler2D uShadowMap;

layout ( set = 1, binding = 1 ) uniform MaterialUniform {
    vec4 uColor;
};

layout ( set = 1, binding = 2 ) uniform sampler2D uColorMap;

layout ( location = 0 ) out vec4 outColor;

const int enablePCF = 0;

float textureProj( vec4 shadowCoord, vec2 offset )
{
	float shadow = 1.0;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) {
		float dist = texture( uShadowMap, vec2( shadowCoord.s, 1.0 - shadowCoord.t ) + offset ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) {
			shadow = 0.25;
		}
	}
	return shadow;
}

float filterPCF( vec4 coord )
{
	ivec2 texDim = textureSize( uShadowMap, 0 );
	float scale = 1.5;
	float dx = scale * 1.0 / float( texDim.x );
	float dy = scale * 1.0 / float( texDim.y );
	float shadowFactor = 0.0f;
	int count = 0;
	int range = 1;
	for ( int y = -range; y <= range; y++ ) {
		for ( int x = -range; x <= range; x++ ) {
			shadowFactor += textureProj( coord, vec2( dx * x, dy * y ) );
			count++;
		}
	}
	return shadowFactor / count;
}

void main()
{
	float shadow = filterPCF( inShadowCoord / inShadowCoord.w );// textureProj( inShadowCoord / inShadowCoord.w, vec2( 0.0 ) );
	
	vec3 color = uColor.rgb * texture( uColorMap, inTextureCoord ).rgb;

	vec3 N = normalize( inNormal );
	vec3 L = normalize( inLightVec );
	vec3 V = normalize( inViewVec );
	vec3 R = normalize( -reflect( L, N ) );
	vec3 ambient = vec3( 0.1, 0.0, 0.1 );
	vec3 diffuse = max( dot( N, L ), 0 ) * color;

	outColor = vec4( ambient + diffuse * shadow, 1.0 );
}

