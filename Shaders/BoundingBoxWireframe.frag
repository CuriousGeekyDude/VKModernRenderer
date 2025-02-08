#version 460


layout(location = 0) out vec4 outColor;

layout(location = 0) in flat uint lv_colorIndex;


vec4 lv_wireframeColors[3] = vec4[](
	vec4(1.f, 0.f, 0.f, 1.f),
	vec4(0.f, 1.f, 0.f, 1.f),
	vec4(1.f, 1.f, 0.f, 1.f)
);

void main()
{
	outColor = lv_wireframeColors[lv_colorIndex];
}