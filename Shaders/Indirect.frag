#version 460 core


#extension GL_EXT_nonuniform_qualifier : require


layout(location = 0) in vec3 uvw;
layout(location = 1) in flat uint lv_matIndex;
layout(location = 2) in vec3 lv_normal;
layout(location = 3) in vec4 lv_worldPos;
layout(location = 4) in vec4 lv_tangent;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 lv_gBufferPosition;
layout(location = 2) out vec4 lv_gBufferNormal;
layout(location = 3) out vec4 lv_gBufferAlbedoSpec;

uint lv_ambientOcclusionMapIncluded = 4;
uint lv_normalMapIncluded = 64;
uint lv_emissiveMapIncluded = 8;
uint lv_albedoMapIncluded = 16;
uint lv_opacityMapIncluded = 128;


struct MaterialData
{
		vec4 m_emissiveColor;
		vec4 m_albedo;
		vec4 m_roughness;
		vec4 m_specular;

		float m_transparencyFactor;
		float m_alphaTest;
		float m_metallicFactor;
		
		uint m_flags;

		int m_ambientOcclusionMap;
		int m_emissiveMap;
		int m_albedoMap;
		int m_metallicRoughnessMap;
		int m_normalMap;
		int m_opacityMap;
};


// layout(binding = 5) readonly buffer MatBO  { uint data[]; } mat_bo;



layout(binding = 0) uniform  UniformBuffer { 

mat4   inMtx; 
vec4 cameraPos; 

float scale;
float bias;
float zNear;
float zFar;
float radius;
float attScale;
float distScale;
uint m_enableDeferred;


} ubo;

layout(binding = 5) readonly buffer MaterialDataBuffer { MaterialData matData[]; } lv_matDataBuffer;

layout(binding = 6) uniform sampler2D lv_textures[256];


void RunAlphaTest(float alpha, float alphaThreshold) 
{
  if (alphaThreshold == 0.0) return;
 
 mat4 thresholdMatrix = 
 mat4(1.0 /17.0,  9.0/17.0,  3.0/17.0, 11.0/17.0,
    13.0/17.0,  5.0/17.0, 15.0/17.0,  7.0/17.0,
    4.0/17.0, 12.0/17.0,  2.0/17.0, 10.0/17.0,
    16.0/17.0,  8.0/17.0, 14.0/17.0,  6.0/17.0);

  int x = int(mod(gl_FragCoord.x, 4.0));
  int y = int(mod(gl_FragCoord.y, 4.0));
  alpha = clamp(alpha - 0.5 * thresholdMatrix[x][y], 0.0, 1.0);

  if (alpha < alphaThreshold) discard;
}





void main()
{
	vec3 lv_n = normalize(lv_normal);
	MaterialData lv_matData = lv_matDataBuffer.matData[lv_matIndex];
	
	
	vec4 lv_emissiveColor = lv_matData.m_emissiveColor;
	vec4 lv_albedo = vec4(1.f, 0.f, 0.f, 0.f);
	vec3 lv_normalSample = lv_n;

	if(0 != (lv_matData.m_flags & lv_normalMapIncluded)) {
		lv_normalSample = normalize(texture(lv_textures[nonuniformEXT(lv_matData.m_normalMap)], uvw.xy).xyz);
	}

	if(0 != (lv_matData.m_flags & lv_albedoMapIncluded)) {
		lv_albedo = texture(lv_textures[nonuniformEXT(lv_matData.m_albedoMap)], uvw.xy);
	}

	//RunAlphaTest(lv_albedo.a, lv_matData.m_alphaTest);

	//if (length(lv_normalSample) > 0.5) {
		//lv_n = perturbNormal(lv_n,normalize(ubo.cameraPos.xyz-lv_worldPos.xyz),lv_normalSample,uvw);
	//}


	//vec3 lv_lightDir = normalize(vec3(-1.f, -1.f, 0.1f));

	//float lv_lightDirDotN = clamp( dot(lv_normalSample, lv_lightDir), 0.3, 1.0 );


	//outColor = vec4(lv_lightDirDotN * lv_albedo.rgb + lv_emissiveColor.rgb, 1.f);

	outColor = lv_albedo*0.7;
	lv_gBufferPosition = lv_worldPos;
	lv_gBufferNormal = vec4(lv_normalSample.xyz, 1.f);
	lv_gBufferAlbedoSpec.rgb = lv_albedo.rgb;
	lv_gBufferAlbedoSpec.a = lv_matData.m_specular.r;
}
