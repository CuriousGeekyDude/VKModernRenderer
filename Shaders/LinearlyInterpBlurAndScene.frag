#version 460


layout(location = 0) out vec4 outColor;


layout (location=0) in vec2 lv_uv;


layout(binding = 0) uniform sampler2D lv_blurImage;
layout(binding = 1) uniform sampler2D lv_hdrScene;

void main()
{
	vec3 hdrColor = texture(lv_hdrScene, lv_uv).rgb;      
    vec3 bloomColor = texture(lv_blurImage, lv_uv).rgb;
    hdrColor = mix(hdrColor,bloomColor, 0.000013f);


    outColor =  vec4(hdrColor, 1.f);
}