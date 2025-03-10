#version 460


layout (location=0) in vec2 lv_uv;

layout(location = 0) out vec4 lv_finalColor;



layout(set = 0, binding = 0) uniform UniformBuffer
{

	mat4   m_inMtx;
	mat4   m_viewMatrix;
	vec4   m_cameraPos;
    vec4   m_time;

}lv_cameraUniform;


struct Light
{
	vec4 m_position;
	vec4 m_color;

	float m_linear;
	float m_quadratic;
	float m_radius;
	float m_pad;
};


uint m_totalNumLights = 1;
const float PI = 3.14159265359;



layout(set = 0, binding = 1) readonly buffer LightData {Light lights[];} lv_lights;


layout(set = 0, binding = 2) uniform sampler2D lv_gbufferPos;
layout(set = 0, binding = 3) uniform sampler2D lv_gbufferNormal;
layout(set = 0, binding = 4) uniform sampler2D lv_gbufferAlbedoSpec;
layout(set = 0, binding = 5) uniform sampler2D lv_gbufferTangent;
layout(set = 0, binding = 6) uniform sampler2D lv_gbufferNormalVertex;
layout(set = 0, binding = 7) uniform sampler2D lv_occlusionFactor;
layout(set = 0, binding = 8) uniform sampler2D lv_gbufferMetallic;
layout(set = 0, binding = 9) uniform samplerCube  lv_depthMapLight;

layout(set = 0,binding = 10) uniform  UniformBuffer2 { 

	mat4 m_viewMatrixSun;
	mat4 m_orthoMatrixSun;
	vec4 m_posSun;

} ubo;

layout(set = 0, binding = 11) uniform sampler2D lv_depth;

void main()
{
}