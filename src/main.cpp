
#include "AllInitialValues.hpp"
#include "GeometryConverter.hpp"
#include "UtilTextureProcessing.hpp"
#include "ProcessSceneMetaData.hpp"
#include "SceneConverter.hpp"
#include "SceneLoaderAndSaver.hpp"
#include "MaterialLoaderAndSaver.hpp"
#include <fstream>

#include "VulkanRenderer.hpp"


/*Before running the renderer, the scene conversion tool needs to run.
To run the tool, define CONVERT_SCENE. Afterward, comment it out and 
compile again and run the renderer.*/

#define CONVERT_SCENE;


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
		
		std::string lv_baseFilePathAssimpScene1 = l_sceneMetaData.m_assimpSceneFileName.substr(0, lv_baseFilePathAssimpSceneCounter+1);



		UtilTextureProcessing::convertAndDownscaleAllTextures(lv_materialLoaderAndSaver.GetMaterials(), 
			lv_baseFilePathAssimpScene1, lv_materialLoaderAndSaver.GetFileNames(), lv_materialLoaderAndSaver.GetOpaqueMapsFiles());
		lv_materialLoaderAndSaver.SaveMaterials();

		printf("\nConversion to material structures and saving them was successful.\n\n");
	}
}


int main()
{

#ifdef CONVERT_SCENE

	ConvertScene();

#else 


	VulkanEngine::VulkanRenderer lv_renderer(VulkanEngine::InitialValues::lv_initialWidthScreen,
		VulkanEngine::InitialValues::lv_intitalHeightScreen, VulkanEngine::InitialValues::lv_frameGraphJSONPath);

	lv_renderer.mainLoop();

#endif

	return 0;
}
