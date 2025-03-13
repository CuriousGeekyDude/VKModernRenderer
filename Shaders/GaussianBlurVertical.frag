#version 460


layout(location = 0) out vec4 outColor;



layout (location=0) in vec2 lv_uv;


layout(set = 0, binding = 0) uniform sampler2D lv_colorImage;


float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);



void main()
{
	
	vec2 tex_offset = 1.0 / textureSize(lv_colorImage, 0); // gets size of single texel
    vec3 result = texture(lv_colorImage, lv_uv).rgb * weight[0]; // current fragment's contribution


	for(int i = 1; i < 5; ++i)
        {
            result += texture(lv_colorImage, lv_uv + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(lv_colorImage, lv_uv - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }

    outColor = vec4(result, 1.f);

}