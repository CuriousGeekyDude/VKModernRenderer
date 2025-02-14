#version 460 core


#extension GL_EXT_nonuniform_qualifier : require


layout(location = 0) in vec3 uvw;
layout(location = 1) in flat uint lv_matIndex;
layout(location = 2) in vec3 lv_normal;
layout(location = 3) in vec4 lv_worldPos;
layout(location = 4) in vec4 lv_tangent;

layout(location = 0) out vec4 lv_gbufferTangent;
layout(location = 1) out vec4 lv_gBufferPosition;
layout(location = 2) out vec4 lv_gBufferNormal;
layout(location = 3) out vec4 lv_gBufferAlbedoSpec;
layout(location = 4) out vec4 lv_gbufferNormalVertex;

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



	lv_gbufferTangent = lv_tangent;
	lv_gBufferPosition = lv_worldPos;
	lv_gBufferNormal = vec4(lv_normalSample.xyz, 1.f);
	lv_gBufferAlbedoSpec.rgb = pow(lv_albedo.rgb, vec3(2.2f));
	lv_gBufferAlbedoSpec.a = lv_matData.m_specular.r;
	lv_gbufferNormalVertex = vec4(lv_n, 1.f);
}
