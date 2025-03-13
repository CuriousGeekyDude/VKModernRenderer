#version 460


layout (location=0) in vec2 lv_uv;

layout(location = 0) out vec4 lv_finalColor;

layout(set = 0, binding = 0) uniform sampler2D lv_colorImage;



void main()
{
	vec4 lv_color = texture(lv_colorImage, lv_uv);
	ivec2 lv_imageSize= textureSize(lv_colorImage, 0);
	vec2 lv_offset = 1.f/vec2(lv_imageSize.xy);

	vec2 lv_boundaries = 1.f - (3.f*lv_offset);


	float lv_brightness = dot(lv_color.rgb, vec3(0.2126, 0.7152, 0.0722));


		if(lv_brightness > 1.f) {
			lv_finalColor = lv_color;
		}
	
	else{
		lv_finalColor = vec4(0.f, 0.f, 0.f, 1.f);
	}
}