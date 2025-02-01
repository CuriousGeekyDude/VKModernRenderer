#pragma once


#include "SceneMetaData.hpp"
#include <vector>

namespace SceneMetaData
{
	class ProcessSceneMetaData final
	{
	public:

		ProcessSceneMetaData(const std::string& l_sceneMetaDataJSONFile);

		const std::vector<SceneMetaData>& GetScenesMetaData() const { return m_scenesMetaData; }

	private:
		std::vector<SceneMetaData> m_scenesMetaData{};
	};
}