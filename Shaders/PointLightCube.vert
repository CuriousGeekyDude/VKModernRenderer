#version 460


layout(location = 0) in vec3 lv_vertex;


layout(set = 0, binding = 0) uniform  UniformBuffer { 

			mat4 m_viewMatrix;
			mat4 m_projectionMatrix;
			vec4 m_cameraPos;

} ubo;



void main()
{
	vec4 lv_worldPos = vec4(lv_vertex, 1.f);

	gl_Position = ubo.m_projectionMatrix * ubo.m_viewMatrix * lv_worldPos;

}