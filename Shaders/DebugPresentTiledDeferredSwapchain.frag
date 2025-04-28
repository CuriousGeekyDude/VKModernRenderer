#version 460


layout(location = 0) out vec4 outColor;


layout (location=0) in vec2 lv_uv;

layout(binding = 0) uniform sampler2D lv_image;



void main()
{
	
	vec3 lv_input = texture(lv_image, lv_uv).rgb;     
	
	
	if(0.f == lv_input.r) {
		outColor = vec4(0.f, 0.f, 0.f, 1.f);

	}
	else if(1.f == lv_input.r) {
		outColor = vec4(0.f, 0.f, 0.4f, 1.f);
	}
	else if(2.f == lv_input.r) {
		outColor = vec4(0.f, 0.f, 0.75f, 1.f);
		
	}
	else if(3.f == lv_input.r) {
		outColor = vec4(0.f, 0.f, 1.f, 1.f);
		
	}
	else if(4.f == lv_input.r) {
		outColor = vec4(0.f, 0.4f, 0.f, 1.f);
		
	}
	else if(5.f <= lv_input.r && lv_input.r <= 7) {
		outColor = vec4(0.f, 0.75f, 0.f, 1.f);
		
	}
	else if(8.f <= lv_input.r && lv_input.r <= 10.f) {
		outColor = vec4(0.f, 1.f, 0.f, 1.f);
		
	}
	else if(11.f <= lv_input.r && lv_input.r <= 15.f) {
		outColor = vec4(1.f, 1.f, 0.f, 1.f);
		
	}
	else if(16.f <= lv_input.r && lv_input.r <= 23.f) {
		outColor = vec4(0.729f, 0.557f, 0.137f, 1.f);
		
	}
	else if(24.f <= lv_input.r && lv_input.r <= 35.f) {
		outColor = vec4(1.f, 0.647f, 0.f, 1.f);
		
	}
	else if(36.f <= lv_input.r && lv_input.r <= 52.f) {
		outColor = vec4(0.75f, 0.f, 0.f, 1.f);

	}
	else if(lv_input.r > 52.f) {
		outColor = vec4(1.f, 0.f, 0.f, 1.f);

	}

}