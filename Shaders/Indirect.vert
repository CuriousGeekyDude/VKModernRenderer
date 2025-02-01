#version 460 core

layout(location = 0) out vec3 uvw;
layout(location = 1) out flat uint lv_matIndex;
layout(location = 2) out vec3 lv_normal;
layout(location = 3) out vec4 lv_worldPos;
layout(location = 4) out vec4 lv_tangent;


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
struct MaterialData { uint tex2D; };


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

void main()
{
	DrawData dd = drawDataBuffer.data[gl_BaseInstance];

	uint refIdx = dd.indexOffset + gl_VertexIndex;
	ImDrawVert v = sbo.data[ibo.data[refIdx] + dd.vertexOffset];

	uvw = vec3(-v.u, -v.v, 1.f);
	lv_matIndex = dd.material;
	lv_worldPos = vec4(v.x, v.y, v.z, 1.0);

	lv_normal = vec3(v.nx, -v.ny, v.nz);
	lv_tangent = vec4(v.tx, v.ty, v.tz, v.tw);

//	mat4 xfrm(1.0); // = transpose(drawDataBuffer.data[gl_BaseInstance].xfrm);

	gl_Position = ubo.inMtx * lv_worldPos;
}
