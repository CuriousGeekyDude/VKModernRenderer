#version 460 core


#extension GL_EXT_nonuniform_qualifier : require



struct ImDrawVert   { float x, y, z; float u, v; float nx, ny, nz;  float tx, ty, tz, tw;};
struct DrawData {
	uint mesh;
	uint material;
	uint lod;
	uint indexOffset;
	uint vertexOffset;
	uint transformIndex;
	uint m_padding2;
	uint m_padding3;
};


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



layout(binding = 1) readonly buffer SBO    { ImDrawVert data[]; } sbo;
layout(binding = 2) readonly buffer IBO    { uint   data[]; } ibo;
layout(binding = 3) readonly buffer DrawBO { DrawData data[]; } drawDataBuffer;
layout(binding = 4) readonly buffer Transformations {mat4 ModelMatrices[];} modelTransformations;


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





layout(binding = 5) readonly buffer MaterialDataBuffer { MaterialData matData[]; } lv_matDataBuffer;

layout(binding = 6) uniform sampler2D lv_textures[256];


void main()
{}

