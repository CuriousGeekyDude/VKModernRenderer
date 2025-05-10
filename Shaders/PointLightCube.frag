#version 460


layout(location = 0) out vec4 outColor;


layout(set = 0, binding = 1) uniform UniformBufferLight
{

	float m_lightIntensity;
	float m_pad1;
	float m_pad2;
	float m_pad3;

}lv_light;



void main()
{
	
	outColor = lv_light.m_lightIntensity*vec4(1.f, 1.f, 1.f, 1.f);

	outColor.a = 1.f;

}