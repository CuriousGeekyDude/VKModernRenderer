#version 460 core


layout(location = 0) in vec3 lv_pos;
layout(location = 1) in float lv_meshID;


layout (location=0) out vec2 uv;



layout(set = 0, binding = 0) uniform UniformBuffer
{

	mat4   m_inMtx;
	mat4   m_viewMatrix;
	vec4   m_cameraPos;

}lv_cameraUniform;



void main() {
  float u = float(((uint(gl_VertexIndex)+2u) / 3u) % 2u);
  float v = float(((uint(gl_VertexIndex)+1u) / 3u) % 2u);
  gl_Position = vec4(-1.0+u*2.0, -1.0+v*2.0, 0., 1.);
  uv = vec2(u, v);

  gl_Position = lv_cameraUniform.m_inMtx * vec4(lv_pos, 1.f);

}