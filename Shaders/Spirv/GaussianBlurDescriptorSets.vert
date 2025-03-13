#version 460


layout(location = 0) out vec4 outColor;


layout (location=0) in vec2 lv_uv;


layout(set = 0, binding = 0) uniform sampler2D lv_colorImage;


float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);



void main(){}