

#include "SceneLoaderAndSaver.hpp"
#include "Hierarchy.hpp"

namespace SceneLoaderAndSaver
{

	SceneLoaderAndSaver::SceneLoaderAndSaver(const std::string& l_sceneFile,
		SceneConverter::Scene& l_customizedScene)
		:m_sceneFile(l_sceneFile), m_cacheProcessedScene(l_customizedScene)
	{
		MarkNode(m_cacheProcessedScene.value(), 0);
		RecalculateGlobalTransforms(m_cacheProcessedScene.value());
	}

	SceneLoaderAndSaver::SceneLoaderAndSaver(const std::string& l_sceneFile)
		:m_sceneFile(l_sceneFile)
	{
		LoadScene(m_scene);
	}




	void SceneLoaderAndSaver::LoadScene(SceneConverter::Scene& l_scene)
	{
		FILE* lv_sceneFile = fopen(m_sceneFile.c_str(), "rb");

		if (nullptr == lv_sceneFile) {
			printf("Scene file failed to load.\n");
			exit(EXIT_FAILURE);
		}

		uint32_t lv_totalNumNodes{};
		if (1 != fread(&lv_totalNumNodes, sizeof(lv_totalNumNodes), 1, lv_sceneFile)) {
			printf("Failed to read from the opened scene file.\n");
			exit(EXIT_FAILURE);
		}

		l_scene.m_globalTransforms.resize(lv_totalNumNodes);
		l_scene.m_hierarchies.resize(lv_totalNumNodes);
		l_scene.m_localTransforms.resize(lv_totalNumNodes);
		
		if (lv_totalNumNodes != fread(l_scene.m_localTransforms.data(), sizeof(glm::mat4), lv_totalNumNodes, lv_sceneFile) ||
			lv_totalNumNodes != fread(l_scene.m_globalTransforms.data(), sizeof(glm::mat4), lv_totalNumNodes, lv_sceneFile) ||
			lv_totalNumNodes != fread(l_scene.m_hierarchies.data(), sizeof(SceneConverter::Hierarchy), lv_totalNumNodes, lv_sceneFile)) {

			printf("Failed to read proper hierarchy data from the opened scene file.\n");
			exit(EXIT_FAILURE);
		}

		LoadUnorderedMap(l_scene.m_meshes, lv_sceneFile);
		LoadUnorderedMap(l_scene.m_materialIDs, lv_sceneFile);

		fclose(lv_sceneFile);
	}

	void SceneLoaderAndSaver::LoadUnorderedMap(std::unordered_map<uint32_t, uint32_t>& l_map, FILE* lv_sceneFile)
	{
		uint32_t lv_totalNumMeshes{};
		std::vector<uint32_t> lv_tempMap{};

		if (1 != fread(&lv_totalNumMeshes, sizeof(uint32_t), 1, lv_sceneFile)) {
			printf("Problem loading total number of map data from the opened scene file.\n");
			exit(EXIT_FAILURE);
		}

		lv_tempMap.resize(2*lv_totalNumMeshes);

		if (2*lv_totalNumMeshes != fread(lv_tempMap.data(), sizeof(uint32_t), 2*lv_totalNumMeshes, lv_sceneFile)) {
			printf("Problem loading map data from the opened scene file.\n");
			exit(EXIT_FAILURE);
		}

		for (uint32_t i = 0; i < lv_totalNumMeshes; ++i) {
			l_map[lv_tempMap[2 * i]] = lv_tempMap[2 * i + 1];
		}
		
	}

	void SceneLoaderAndSaver::SaveUnorderedMap(const std::unordered_map<uint32_t, 
		uint32_t>& l_map, FILE* lv_sceneFile)
	{
		std::vector<uint32_t> lv_tempMapData{};
		lv_tempMapData.reserve(l_map.size()*2);

		for (auto& l_mapPair : l_map) {
			lv_tempMapData.push_back(l_mapPair.first);
			lv_tempMapData.push_back(l_mapPair.second);
		}

		if (lv_tempMapData.size() != fwrite(lv_tempMapData.data(), sizeof(uint32_t), lv_tempMapData.size(), lv_sceneFile)) {
			printf("Problem saving mapped data into scene file.\n");
			exit(EXIT_FAILURE);
		}

	}


	void SceneLoaderAndSaver::MarkNode(SceneConverter::Scene& l_scene,uint32_t l_nodeToMark)
	{
		auto lv_levelOfNodeToMark = l_scene.m_hierarchies[l_nodeToMark].m_level;



		if (lv_levelOfNodeToMark < SceneConverter::MAX_SCENE_LEVELS) {

			l_scene.m_markedNodes[lv_levelOfNodeToMark].push_back(l_nodeToMark);

			for (uint32_t i = l_scene.m_hierarchies.at(l_nodeToMark).m_firstChild; i != -1; i = l_scene.m_hierarchies[i].m_nextSibling) {
				MarkNode(l_scene,i);
			}

		}
	}


	void SceneLoaderAndSaver::RecalculateGlobalTransforms(SceneConverter::Scene& l_scene)
	{
		if (false == l_scene.m_markedNodes[0].empty()) {
			
			auto lv_parentIndex = l_scene.m_markedNodes[0][0];
			l_scene.m_globalTransforms[lv_parentIndex] = l_scene.m_localTransforms[lv_parentIndex];
			l_scene.m_markedNodes[0].clear();

			for (uint32_t i = 1U; 
				i < SceneConverter::MAX_SCENE_LEVELS && false == l_scene.m_markedNodes[i].empty();
				++i) {
				for (auto lv_nodeIndex : l_scene.m_markedNodes[i]) {
					l_scene.m_globalTransforms[lv_nodeIndex] =
						l_scene.m_globalTransforms[l_scene.m_hierarchies[lv_nodeIndex].m_parent] * l_scene.m_localTransforms[lv_nodeIndex];
				}

				l_scene.m_markedNodes[i].clear();
			}
		}
	}


	void SceneLoaderAndSaver::SaveScene(SceneConverter::Scene& l_scene)
	{
		FILE* lv_sceneFile = fopen(m_sceneFile.c_str(), "wb");

		if (nullptr == lv_sceneFile) {
			printf("Failed to open scene file in order to save the current scene into in.\n");
			exit(EXIT_FAILURE);
		}

		uint32_t lv_totalNumNodes{ (uint32_t)l_scene.m_hierarchies.size() };

		if (1 != fwrite(&lv_totalNumNodes, sizeof(uint32_t), 1, lv_sceneFile)) {
			printf("Failed to save total number of nodes to scene file.\n");
			exit(EXIT_FAILURE);
		}

		if (lv_totalNumNodes != fwrite(l_scene.m_localTransforms.data(), sizeof(glm::mat4), lv_totalNumNodes, lv_sceneFile) ||
			lv_totalNumNodes != fwrite(l_scene.m_globalTransforms.data(), sizeof(glm::mat4), lv_totalNumNodes, lv_sceneFile) ||
			lv_totalNumNodes != fwrite(l_scene.m_hierarchies.data(), sizeof(SceneConverter::Hierarchy), lv_totalNumNodes, lv_sceneFile)) {
			printf("Failed to save hierarchy data to the scene file.\n");
			exit(EXIT_FAILURE);
		}


		uint32_t lv_totalNumMeshes{(uint32_t)l_scene.m_meshes.size()};

		if (1 != fwrite(&lv_totalNumMeshes, sizeof(uint32_t), 1, lv_sceneFile)) {
			printf("Failed to save total number of meshes to the scene file.\n");
			exit(EXIT_FAILURE);
		}

		SaveUnorderedMap(l_scene.m_meshes, lv_sceneFile);
		SaveUnorderedMap(l_scene.m_materialIDs, lv_sceneFile);
	}
}