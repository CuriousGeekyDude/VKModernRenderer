#version 460


layout(location = 0) in vec3 lv_vertex;



layout(set = 0, binding = 0) uniform  UniformBuffer { 

mat4   inMtx; 
mat4   viewMatrix;
vec4 cameraPos; 

} ubo;


layout(set = 0, binding = 1) readonly buffer wireframeObjectIDs {uint Ids[]; } lv_wireFrameIDs;


layout(location = 0) out vec4 outColor;


void main(){}