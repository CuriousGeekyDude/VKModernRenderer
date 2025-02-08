#version 460


layout(location = 0) in vec3 lv_vertex;
layout(location = 1) in float lv_colorID;

layout(location = 0) out uint lv_colorIndex;


layout(set = 0, binding = 0) uniform  UniformBuffer { 

mat4   inMtx; 
mat4   viewMatrix;
vec4 cameraPos; 

} ubo;



layout(set = 0, binding = 1) readonly buffer wireframeObjectIDs {uint Ids[]; } lv_wireFrameIDs;

void main()
{
	vec4 lv_worldPos = vec4(lv_vertex, 1.f);

	gl_Position = ubo.inMtx * lv_worldPos;

	lv_colorIndex = lv_wireFrameIDs.Ids[uint(lv_colorID)];

}