#pragma once


#include <stdint.h>

namespace RenderCore
{
	struct alignas(16) InstanceData
	{
		uint32_t m_meshIndex;
		uint32_t m_materialIndex;
		uint32_t m_lod;
		uint32_t m_indexBufferIndex;
		uint32_t m_vertexBufferIndex;
		uint32_t m_transformIndex;
		
	};
}