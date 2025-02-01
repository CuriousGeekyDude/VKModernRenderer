#pragma once


#include <cinttypes>


namespace SceneConverter
{
	struct Hierarchy
	{
		uint32_t m_level{};
		int m_nextSibling{};
		int m_firstChild{};
		int m_lastSibling{};
		int m_parent{};
	};
}