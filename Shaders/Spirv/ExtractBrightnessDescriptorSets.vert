#version 460


layout (location=0) in vec2 lv_uv;

layout(location = 0) out vec4 lv_finalColor;

layout(set = 0, binding = 0) uniform sampler2D lv_swapchainImage;



void main()
{
	vec4 lv_color = texture(lv_swapchainImage, lv_uv);

	float lv_brightness = dot(lv_color.rgb, vec3(0.2126, 0.7152, 0.0722));

	if(lv_brightness > 1.f) {
		lv_finalColor = lv_color;
	}
	else{
		lv_finalColor = vec4(0.f, 0.f, 0.f, 1.f);
	}
}
