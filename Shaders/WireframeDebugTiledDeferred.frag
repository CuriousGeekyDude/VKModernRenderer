#version 460



layout(location = 0) in flat uint lv_colorIndex;

layout(location = 0) out vec4 lv_color;


vec4 lv_colorArray[2] = vec4[](
    vec4(1.0, 0.0, 0.0, 1.0),
    vec4(0.0, 1.0, 0.0, 1.0) 
);


void main()
{

	lv_color = lv_colorArray[lv_colorIndex];
}