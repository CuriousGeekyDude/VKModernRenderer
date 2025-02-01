#pragma once



#include "Scene.hpp"
#include <string>
#include <unordered_map>
#include <optional>

namespace SceneLoaderAndSaver
{
	class SceneLoaderAndSaver final
	{

	public:

		SceneLoaderAndSaver(const std::string& l_sceneFile, 
			SceneConverter::Scene& l_customizedScene);

		SceneLoaderAndSaver(const std::string& l_sceneFile);


		SceneConverter::Scene& GetScene() { return m_scene; }
		SceneConverter::Scene& GetCachedScene() { return m_cacheProcessedScene.value(); }


		void SaveScene(SceneConverter::Scene& l_scene);

		void MarkNode(SceneConverter::Scene& l_scene,uint32_t l_nodeToMark);
		void RecalculateGlobalTransforms(SceneConverter::Scene& l_scene);

		void LoadScene(SceneConverter::Scene& l_scene);


	private:

		


		void LoadUnorderedMap(std::unordered_map<uint32_t, uint32_t>& l_map, FILE* lv_sceneFile);
		void SaveUnorderedMap(const std::unordered_map<uint32_t, uint32_t>& l_map, FILE* lv_sceneFile);

	private:

		SceneConverter::Scene m_scene;
		std::optional<SceneConverter::Scene> m_cacheProcessedScene;
		const std::string m_sceneFile;
	};
}