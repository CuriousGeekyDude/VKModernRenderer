#version 460


layout(location = 0) out uint lv_colorIndex;

layout(location = 0) in vec4 lv_pos;



void main()
{
	gl_Position = vec4(lv_pos.x,  lv_pos.y , 0.f, 1.f);

	lv_colorIndex = uint(lv_pos.w);
}