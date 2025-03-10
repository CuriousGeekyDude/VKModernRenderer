#version 460 core


layout(binding = 0) uniform  UniformBuffer { 

	mat4 m_viewMatrix;
	mat4 m_projMatrix;
	vec4 m_pos;

} ubo;



layout(location = 0) in vec4 lv_worldPos;


const float lv_farPlane = 100.f;

void main()
{
	float lv_distFromLightToFrag = length(lv_worldPos.xyz - ubo.m_pos.xyz);

	lv_distFromLightToFrag = 1/lv_farPlane * lv_distFromLightToFrag;

	gl_FragDepth = lv_distFromLightToFrag;

}