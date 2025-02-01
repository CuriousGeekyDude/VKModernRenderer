#pragma once


#include <stdint.h>


namespace MeshConverter
{

	struct MeshFileHeader final
	{
		uint32_t m_magicValue{0X12345678};
		uint32_t m_meshCount{};
		uint32_t m_startBlockOffset{};
		uint32_t m_vertexDataSize{};
		uint32_t m_lodDataSize{};
	};

}