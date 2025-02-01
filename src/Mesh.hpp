#pragma once

#include <stdint.h>
#include <cassert>

namespace MeshConverter
{
	constexpr const uint32_t lv_maxLODCount{ 8 };
	constexpr const uint32_t lv_maxStreamCount{ 8 };

	struct Mesh final
	{

		uint32_t m_lodCount{};
		uint32_t m_streamCount{};
		
		uint32_t m_materialID{};

		uint32_t m_lodOffsets[lv_maxLODCount];
		uint32_t m_streamOffsets[lv_maxStreamCount];

		uint32_t m_meshSize{};
		uint32_t m_vertexCount{};

		uint32_t m_streamElementSizes[lv_maxStreamCount];



		inline uint32_t CalculateLODSize(uint32_t l_lodOffsetIndex) const
		{
			assert(l_lodOffsetIndex + 1 < lv_maxLODCount);
			return (m_lodOffsets[l_lodOffsetIndex + 1] - m_lodOffsets[l_lodOffsetIndex]);
		}

		inline uint32_t CalculateLODNumberOfIndices(uint32_t l_lodOffsetIndex) const
		{
			assert(l_lodOffsetIndex + 1 < lv_maxLODCount);
			return (m_lodOffsets[l_lodOffsetIndex + 1] - m_lodOffsets[l_lodOffsetIndex])/sizeof(unsigned int);
		}
	};
}