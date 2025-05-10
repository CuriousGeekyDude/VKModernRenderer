
#include "AllInitialValues.hpp"
#include "GeometryConverter.hpp"
#include "UtilTextureProcessing.hpp"
#include "ProcessSceneMetaData.hpp"
#include "SceneConverter.hpp"
#include "SceneLoaderAndSaver.hpp"
#include "MaterialLoaderAndSaver.hpp"
#include <fstream>
#include <filesystem>

#include "VulkanRenderer.hpp"




void ConvertScene()
{


	SceneMetaData::ProcessSceneMetaData lv_sceneMetaDataProcessor{ VulkanEngine::InitialValues::lv_sceneJSONPath };
	MeshConverter::GeometryConverter lv_geometryConverter{};
	auto lv_sceneMetaDatas = lv_sceneMetaDataProcessor.GetScenesMetaData();


	for (uint32_t i = 0; i < lv_sceneMetaDatas.size(); ++i) {

		auto& l_sceneMetaData = lv_sceneMetaDatas[i];
		lv_geometryConverter.ConvertScene(l_sceneMetaData.m_assimpSceneFileName, l_sceneMetaData.m_outputMesh,
			l_sceneMetaData.m_outputBoxes, l_sceneMetaData.m_outputInstanceData, true, true, true);

		SceneLoaderAndSaver::SceneLoaderAndSaver lv_sceneLoaderAndSaver(l_sceneMetaData.m_outputScene, lv_geometryConverter.GetScene());
		lv_sceneLoaderAndSaver.SaveScene(lv_sceneLoaderAndSaver.GetCachedScene());


		printf("\n\n-----------------\\n");
		printf("\nProcess of converting to customized material structure begins.\n\n");
		MaterialLoaderAndSaver::MaterialLoaderAndSaver lv_materialLoaderAndSaver{ l_sceneMetaData.m_outputMaterials };

		for (uint32_t i = 0; i < lv_geometryConverter.GetCurrentaiScene()->mNumMaterials; ++i) {
			lv_materialLoaderAndSaver.ConvertAiMaterialToMaterial(lv_geometryConverter.GetCurrentaiScene()->mMaterials[i]);
		}

		uint32_t lv_baseFilePathAssimpSceneCounter = l_sceneMetaData.m_assimpSceneFileName.find_last_of("/");

		std::string lv_baseFilePathAssimpScene1 = l_sceneMetaData.m_assimpSceneFileName.substr(0, lv_baseFilePathAssimpSceneCounter + 1);



		UtilTextureProcessing::convertAndDownscaleAllTextures(lv_materialLoaderAndSaver.GetMaterials(),
			lv_baseFilePathAssimpScene1, lv_materialLoaderAndSaver.GetFileNames(), lv_materialLoaderAndSaver.GetOpaqueMapsFiles());
		lv_materialLoaderAndSaver.SaveMaterials();

		printf("\nConversion to material structures and saving them was successful.\n\n");
	}
}


int main()
{

	std::string lv_path = "Assets/";
	int lv_fileCount = 0;

	if (std::filesystem::exists(lv_path) && std::filesystem::is_directory(lv_path)) {
		for (const auto& entry : std::filesystem::directory_iterator(lv_path)) {
			if (std::filesystem::is_regular_file(entry.status())) {
				++lv_fileCount;
			}
		}
	}


	if (lv_fileCount <= 0) {
		ConvertScene();
	}


	VulkanEngine::VulkanRenderer lv_renderer(VulkanEngine::InitialValues::lv_initialWidthScreen,
		VulkanEngine::InitialValues::lv_intitalHeightScreen, VulkanEngine::InitialValues::lv_frameGraphJSONPath);

	lv_renderer.mainLoop();

	return 0;
}

