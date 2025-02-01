


#include "ProcessSceneMetaData.hpp"
#include <fstream>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <iostream>

namespace SceneMetaData
{
	ProcessSceneMetaData::ProcessSceneMetaData(const std::string& l_sceneMetaDataJSONFile)
	{
        std::ifstream lv_sceneMetaDataJSONStream(l_sceneMetaDataJSONFile);
        rapidjson::IStreamWrapper lv_isw(lv_sceneMetaDataJSONStream);

        rapidjson::Document lv_document;
        const rapidjson::ParseResult lv_parseResult= lv_document.ParseStream(lv_isw);

        if (lv_parseResult.IsError() == true) {
            std::cout << lv_parseResult.Code() << std::endl;
            exit(-1);
        }

        for (rapidjson::SizeType i = 0; i < lv_document.Size(); i++) {

            m_scenesMetaData.emplace_back(SceneMetaData{
              .m_assimpSceneFileName = lv_document[i]["input_scene"].GetString(),
              .m_outputMesh = lv_document[i]["output_mesh"].GetString(),
              .m_outputScene = lv_document[i]["output_scene"].GetString(),
              .m_outputMaterials = lv_document[i]["output_materials"].GetString(),
              .m_outputBoxes = lv_document[i]["output_boundingBoxes"].GetString(),
              .m_outputInstanceData = lv_document[i]["output_instanceData"].GetString(),
              .m_scale = (float)lv_document[i]["scale"].GetDouble(),
              .m_mergeInstances = lv_document[i]["merge_instances"].GetBool()
            });
        }
	}
}