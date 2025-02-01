#pragma once



#include "Hierarchy.hpp"
#include <glm/glm.hpp>
#include <unordered_map>
#include <array>

namespace SceneConverter
{
	constexpr uint32_t MAX_SCENE_LEVELS = 16U;

	struct Scene
	{

		std::vector<glm::mat4> m_localTransforms{};
		std::vector<glm::mat4> m_globalTransforms{};


		//Nodes marked to recalculate their global transformation
		std::array<std::vector<uint32_t>, MAX_SCENE_LEVELS> m_markedNodes{};


		std::vector<Hierarchy> m_hierarchies{};
		
		std::unordered_map<uint32_t, uint32_t> m_meshes{};
		std::unordered_map<uint32_t, uint32_t> m_materialIDs{};

	};
}


