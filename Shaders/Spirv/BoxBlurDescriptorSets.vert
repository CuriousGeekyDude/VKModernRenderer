#version 460 core



layout(location = 0) in vec2 lv_uv;

layout(location = 0) out float lv_color;



layout(set = 0, binding = 0) uniform sampler2D lv_ssao;



void main()
{
	vec2 texelSize = 1.0 / vec2(textureSize(lv_ssao, 0));
    float result = 0.0;
    for (int x = -2; x < 2; ++x) 
    {
        for (int y = -2; y < 2; ++y) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(lv_ssao, lv_uv + offset).r;
        }
    }
    lv_color = result / (4.0 * 4.0);
}