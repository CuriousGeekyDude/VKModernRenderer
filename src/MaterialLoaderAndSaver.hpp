#pragma once



#include "Material.hpp"
#include <string>
#include <vector>
#include <assimp/material.h>
#include <assimp/pbrmaterial.h>


namespace MaterialLoaderAndSaver
{
	class MaterialLoaderAndSaver final
	{
	public:

		MaterialLoaderAndSaver(const std::string& l_materialFileName);

		void LoadMaterialFile();
		void SaveMaterials();

		SceneConverter::Material ConvertAiMaterialToMaterial(const aiMaterial* l_material);

		const std::vector<SceneConverter::Material>& GetMaterials() const { return m_materials; }
		std::vector<std::string>& GetFileNames() { return m_files; }
		std::vector<std::string>& GetOpaqueMapsFiles() { return m_opaqueMapFiles; }


	private:
		int AddStringToContainer(std::vector<std::string>& lv_stringContainer, const std::string& lv_string);

	private:
		std::vector<SceneConverter::Material> m_materials{};
		std::vector<std::string> m_files{};
		std::vector<std::string> m_opaqueMapFiles{};
		const std::string m_materialFileName{};
	};
}