#version 460


layout(location = 0) out vec4 outColor;


layout (location=0) in vec2 lv_uv;

layout(binding = 0) uniform sampler2D lv_image;



void main()
{
	
	vec3 hdrColor = texture(lv_image, lv_uv).rgb;     
	
	hdrColor /= vec3(1.f) - exp(-hdrColor*2.5f);

	hdrColor = pow(hdrColor, vec3(1.f/2.2f));


	outColor = vec4(hdrColor.xyz, 1.f);

}