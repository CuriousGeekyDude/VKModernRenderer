
#version 460 core



layout (location=0) in vec2 uv;

layout(location = 0) out vec4 lv_outColor;

layout(set = 0, binding = 0) uniform sampler2D lv_outputImageCompute;



void main()
{
	lv_outColor = vec4(texture(lv_outputImageCompute, uv).rgb, 1.f);
}

