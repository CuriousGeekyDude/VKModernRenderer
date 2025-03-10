#version 460 core



layout(location = 0) in vec2 lv_texCoord;



layout(location = 0) out float lv_occlusion;



layout(set = 0, binding = 0) uniform UniformBufferMatrix
{

	mat4 m_viewMatrix;
	mat4 m_perspectiveMatrix;

} uboMatrix;


const uint lv_offsetBufferSize = 32;
layout(set = 0, binding = 1) readonly buffer OffsetsBuffer {vec4 OffsetData[];} lv_offsetBuffer;


layout(set = 0, binding = 2) uniform sampler2D lv_gbufferPos;
layout(set = 0, binding = 3) uniform sampler2D lv_gbufferNormalVertex;
layout(set = 0, binding = 4) uniform sampler2D lv_randomRotations;


const float lv_radius = 8.f;

void main()
{
	ivec2 lv_textureSize = textureSize(lv_gbufferPos, 0);
	const vec2 lv_rotationScale = vec2(lv_textureSize.x/4.f, lv_textureSize.y/4.f);
	vec3 lv_posInView = texture(lv_gbufferPos, lv_texCoord).rgb;
	vec3 lv_normalInView = texture(lv_gbufferNormalVertex, lv_texCoord).rgb;
	vec3 lv_rotation = texture(lv_randomRotations, lv_texCoord * lv_rotationScale).rgb;

	lv_posInView = (uboMatrix.m_viewMatrix * vec4(lv_posInView, 1.f)).xyz;
	lv_normalInView = (uboMatrix.m_viewMatrix * vec4(lv_normalInView, 0.f)).xyz;
	lv_rotation = (uboMatrix.m_viewMatrix * vec4(lv_rotation, 0.f)).xyz;

	vec3 tangent   = normalize(lv_rotation - lv_normalInView * dot(lv_rotation, lv_normalInView));
	vec3 bitangent = cross(lv_normalInView, tangent);
	mat3 TBN       = mat3(tangent, bitangent, lv_normalInView);

	lv_occlusion = 0.f;

	for(uint i = 0; i < lv_offsetBufferSize; ++i) {
		
		vec3 lv_offset = TBN * lv_offsetBuffer.OffsetData[i].xyz;
		lv_offset = lv_posInView + lv_radius*lv_offset;
		
		vec4 lv_texCoordOffset = vec4(lv_offset, 1.f);
		lv_texCoordOffset = uboMatrix.m_perspectiveMatrix * lv_texCoordOffset;
		lv_texCoordOffset = (1.f/lv_texCoordOffset.w) * lv_texCoordOffset;
		lv_texCoordOffset.xy = lv_texCoordOffset.xy * 0.5f + 0.5f;

		float lv_sampleDepth = (uboMatrix.m_viewMatrix * texture(lv_gbufferPos, lv_texCoordOffset.xy)).z;

		float lv_rangeCheck = smoothstep(0.f, 1.f, lv_radius / abs(lv_sampleDepth - lv_posInView.z));
		lv_occlusion += (lv_sampleDepth >= lv_offset.z + 0.025f ? 1.f : 0.f) * lv_rangeCheck;

	}

	lv_occlusion = 1.f - (lv_occlusion/lv_offsetBufferSize);
}