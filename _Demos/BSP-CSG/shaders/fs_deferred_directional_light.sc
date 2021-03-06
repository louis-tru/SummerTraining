$input v_texcoord0

//based on
/*
 * Copyright 2011-2015 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include "common.sh"

SAMPLER2D(s_normal, 0);
SAMPLER2D(s_depth,  1);

uniform mat4 u_inverseViewMat;


uniform vec4 u_lightVector;	// in view space
uniform vec4 u_lightColor;

vec2 blinn(vec3 _lightDir, vec3 _normal, vec3 _viewDir)
{
	float ndotl = dot(_normal, _lightDir);
	vec3 reflected = _lightDir - 2.0*ndotl*_normal; // reflect(_lightDir, _normal);
	float rdotv = dot(reflected, _viewDir);
	return vec2(ndotl, rdotv);
}

float fresnel(float _ndotl, float _bias, float _pow)
{
	float facing = (1.0 - _ndotl);
	return max(_bias + (1.0 - _bias) * pow(facing, _pow), 0.0);
}

vec4 lit(float _ndotl, float _rdotv, float _m)
{
	float diff = max(0.0, _ndotl);
	float spec = step(0.0, _ndotl) * max(0.0, _rdotv * _m);
	return vec4(1.0, diff, spec, 1.0);
}

float toClipSpaceDepth(float _depthTextureZ)
{
#if BGFX_SHADER_LANGUAGE_HLSL
	return _depthTextureZ;
#else
	return _depthTextureZ * 2.0 - 1.0;
#endif // BGFX_SHADER_LANGUAGE_HLSL
}

vec3 clipToWorld(mat4 _invViewProj, vec3 _clipPos)
{
	vec4 wpos = mul(_invViewProj, vec4(_clipPos, 1.0) );
	return wpos.xyz / wpos.w;
}

// Converts hardware depth buffer value (Z / W) into (1 / W).
// This is used for restoring view-space depth and position.
float HardwareDepthToInverseW( float z )
{
	return 1 / (z * u_invProj._m32 + u_invProj._m33);
}

void main()
{
	vec3 viewNormal = decodeNormalUint(texture2D(s_normal, v_texcoord0).xyz);
	viewNormal = normalize(viewNormal);

	vec3 worldNormal = normalize(mul( u_inverseViewMat, vec4(viewNormal, 0.0) ).xyz);
	
	vec3 worldLightVec = vec3(0,1,0);
	vec3 viewLightVec = normalize(mul( u_view, vec4(worldLightVec, 0.0) ).xyz);

	
	
//	vec3 rgb = normalize(mul(u_invView, vec4(viewNormal, 0.0) ).xyz);
	vec3 rgb = normalize(mul( u_inverseViewMat, vec4(viewNormal, 0.0) ).xyz);
//	vec3 rgb = viewNormal;

//	float NdotL = max(dot(viewNormal, u_lightVector.xyz), 0.0);
	float NdotL = max(dot(worldNormal, worldLightVec), 0.0);
	vec3 lightColor = u_lightColor.rgb * NdotL;
	rgb = lightColor;

#if 0
	vec3 wnormal = normalize(mul(u_inverseViewMat, vec4(viewNormal, 0.0) ).xyz);
	
//	float d = dot(normal,u_lightVector);
//	d = clamp(d,0,1);
//	float d = max(dot(normal,-u_lightVector),0);

	float NdotL = max(dot(viewNormal, u_lightVector.xyz), 0.0);

	vec3 rgb = u_lightColor.rgb * NdotL;
//vec3 rgb = u_lightColor.xyz;


	rgb = viewNormal;
//	rgb = texture2D(s_depth,  v_texcoord0).rgb;

	

#if 0
	float deviceDepth = texture2D(s_depth,  v_texcoord0).x;
	float depth       = toClipSpaceDepth(deviceDepth);

	vec3 clip = vec3(v_texcoord0 * 2.0 - 1.0, depth);
#if BGFX_SHADER_LANGUAGE_HLSL
	clip.y = -clip.y;
#endif // BGFX_SHADER_LANGUAGE_HLSL

	// recover view-space depth
	float3 viewSpacePosition = vec3( _inputs.viewPosition.x, 1, _inputs.viewPosition.y )
								* HardwareDepthToInverseW( deviceDepth );


	vec3 wpos = clipToWorld(u_invViewProj, clip);

	vec3 eyePos = u_invView[3].xyz;
	vec3 viewDir = normalize(wpos - eyePos);
	vec2 bln = blinn(u_lightVector, normal, viewDir);
	vec4 lc = lit(bln.x, bln.y, 1.0);
	vec3 rgb = u_lightColor.xyz * saturate(lc.y);
#endif // 0
	
#endif
	
	gl_FragColor.xyz = toGamma(rgb.xyz);
	gl_FragColor.w = 1.0;
}
