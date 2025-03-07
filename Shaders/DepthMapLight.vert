#version 460 core


layout(binding = 0) uniform  UniformBuffer { 

	mat4 m_viewMatrix;
	mat4 m_orthoMatrix;
	vec4 m_pos;

} ubo;


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


layout(binding = 1) readonly buffer SBO    { ImDrawVert data[]; } sbo;
layout(binding = 2) readonly buffer IBO    { uint   data[]; } ibo;
layout(binding = 3) readonly buffer DrawBO { DrawData data[]; } drawDataBuffer;




void main()
{
	DrawData dd = drawDataBuffer.data[gl_BaseInstance];

	uint refIdx = dd.indexOffset + gl_VertexIndex;
	ImDrawVert v = sbo.data[ibo.data[refIdx] + dd.vertexOffset];

	vec4 lv_worldPos = vec4(v.x, v.y, v.z, 1.0);

	gl_Position = ubo.m_orthoMatrix * ubo.m_viewMatrix * lv_worldPos;
}