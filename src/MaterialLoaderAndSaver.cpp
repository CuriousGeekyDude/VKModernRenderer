


#include "MaterialLoaderAndSaver.hpp"
#include "ErrorCheck.hpp"

namespace MaterialLoaderAndSaver
{

	MaterialLoaderAndSaver::MaterialLoaderAndSaver(const std::string& l_materialFileName) 
		: m_materialFileName(l_materialFileName)
	{
	}

	void MaterialLoaderAndSaver::LoadMaterialFile()
	{
		using namespace SceneConverter;

		FILE* lv_materialFile = fopen(m_materialFileName.c_str(), "rb");

		if (nullptr == lv_materialFile) {
			printf("Material file failed to open.\n");
			exit(EXIT_FAILURE);
		}

		uint32_t lv_totalNumMaterials{};

		if (1 != fread(&lv_totalNumMaterials, sizeof(uint32_t), 1, lv_materialFile)) {
			printf("Total number of material structures failed to be read.\n");
			exit(EXIT_FAILURE);
		}

		m_materials.resize(lv_totalNumMaterials);

		if (lv_totalNumMaterials != fread(m_materials.data(), sizeof(Material), lv_totalNumMaterials, lv_materialFile)) {
			printf("Material structures failed to be read.\n");
			exit(EXIT_FAILURE);
		}

		uint32_t lv_totalNumFiles{};

		if (1 != fread(&lv_totalNumFiles, sizeof(uint32_t), 1, lv_materialFile)) {
			printf("Total number of file strings failed to be read.\n");
			exit(EXIT_FAILURE);
		}



		m_files.resize(lv_totalNumFiles);

		for (auto& l_fileName : m_files) {

			uint32_t lv_totalNumChars{};

			if (1 != fread(&lv_totalNumChars, sizeof(uint32_t), 1, lv_materialFile)) {
				printf("Total number of characters of the file string failed to be read.\n");
				exit(EXIT_FAILURE);
			}

			std::vector<char> lv_rawCharDataFile{};
			lv_rawCharDataFile.resize(lv_totalNumChars+1);
			lv_rawCharDataFile[lv_totalNumChars] = '\0';
			if (lv_totalNumChars != fread(lv_rawCharDataFile.data(), 1, lv_totalNumChars, lv_materialFile)) {
				printf("Raw characters of a file string failed to be read.\n");
				exit(EXIT_FAILURE);
			}

			l_fileName = std::string{ lv_rawCharDataFile.data() };
		}

		

		fclose(lv_materialFile);
	}

	void MaterialLoaderAndSaver::SaveMaterials()
	{
		using namespace SceneConverter;
		using namespace ErrorCheck;

		FILE* lv_materialFile = fopen(m_materialFileName.c_str(), "wb");

		if (nullptr == lv_materialFile) {
			printf("Material file name failed to open for saving.\n");
			exit(EXIT_FAILURE);
		}
		
		uint32_t lv_totalNumMaterials{(uint32_t)m_materials.size()};
		if (1 != fwrite(&lv_totalNumMaterials, sizeof(uint32_t), 1, lv_materialFile)) {
			printf("Failed to write the total number of materials to the material file.\n");
			exit(EXIT_FAILURE);
		}

		if (lv_totalNumMaterials != fwrite(m_materials.data(), sizeof(Material), lv_totalNumMaterials, lv_materialFile)) {
			printf("Failed to write the material structures to the material file.\n");
			exit(EXIT_FAILURE);
		}



		uint32_t lv_totalNumFiles = (uint32_t)(m_files.size());
		
		if (1 != fwrite(&lv_totalNumFiles, sizeof(uint32_t), 1, lv_materialFile)) {
			PRINT_EXIT("Total number of files could not be written to the binary material file\n");
		}

		for (auto& l_fileName : m_files) {

			uint32_t lv_fileNameSize = (uint32_t)(l_fileName.size());

			if (0 == lv_fileNameSize) { continue; }

			if (1 != fwrite(&lv_fileNameSize, sizeof(uint32_t), 1, lv_materialFile)) {
				printf("\nFailed to write the number of characters of the file name to material file. \n");
				exit(EXIT_FAILURE);
			}

			if (lv_fileNameSize != fwrite(l_fileName.data(), 1, lv_fileNameSize, lv_materialFile)) {
				printf("\nFailed to write file string to the material file. \n");
				exit(EXIT_FAILURE);
			}

		}
		


		fclose(lv_materialFile);
	}


	SceneConverter::Material MaterialLoaderAndSaver::ConvertAiMaterialToMaterial(const aiMaterial* l_material)
	{
		using namespace SceneConverter;

		Material lv_outputMaterial{};
		aiColor4D lv_color{};

		if (AI_SUCCESS == aiGetMaterialColor(l_material, AI_MATKEY_COLOR_DIFFUSE, &lv_color)) {
			lv_outputMaterial.m_albedo = glm::vec4{lv_color.r, lv_color.g, lv_color.b, lv_color.a};

			if (lv_outputMaterial.m_albedo.a > 1.f) {
				lv_outputMaterial.m_albedo.a = 1.f;
			}
		}

		if (AI_SUCCESS == aiGetMaterialColor(l_material, AI_MATKEY_COLOR_AMBIENT, &lv_color)) {
			lv_outputMaterial.m_emissiveColor = glm::vec4{ lv_color.r, lv_color.g, lv_color.b, lv_color.a };

			if (lv_outputMaterial.m_emissiveColor.a > 1.f) {
				lv_outputMaterial.m_emissiveColor.a = 1.f;
			}
		}

		if (AI_SUCCESS == aiGetMaterialColor(l_material, AI_MATKEY_COLOR_SPECULAR, &lv_color)) {
			lv_outputMaterial.m_specular = glm::vec4{ lv_color.r, lv_color.g, lv_color.b, lv_color.a };
		}

		if (AI_SUCCESS == aiGetMaterialColor(l_material, AI_MATKEY_COLOR_EMISSIVE, &lv_color)) {
			lv_outputMaterial.m_emissiveColor += glm::vec4{ lv_color.r, lv_color.g, lv_color.b, lv_color.a };

			if (lv_outputMaterial.m_emissiveColor.a > 1.f) {
				lv_outputMaterial.m_emissiveColor.a = 1.f;
			}
		}
		
		float lv_opacity{};
		if (AI_SUCCESS == aiGetMaterialFloat(l_material, AI_MATKEY_OPACITY, &lv_opacity)) {

			lv_outputMaterial.m_transparencyFactor =
				glm::clamp(1.0f - lv_opacity, 0.0f, 1.0f);

			if (lv_outputMaterial.m_transparencyFactor >= 0.95f) {
				lv_outputMaterial.m_transparencyFactor = 0.0f;
			}
		}

		if (AI_SUCCESS == aiGetMaterialColor(l_material, AI_MATKEY_COLOR_TRANSPARENT, &lv_color)) {

			float Opacity = std::max(std::max(lv_color.r, lv_color.g), lv_color.b);
			lv_outputMaterial.m_transparencyFactor = glm::clamp(Opacity, 0.0f, 1.0f);
			
			if (lv_outputMaterial.m_transparencyFactor >= 0.95f) {
				lv_outputMaterial.m_transparencyFactor = 0.0f;
			}
			lv_outputMaterial.m_alphaTest = 0.5f;
		}

		float lv_tmp{ 1.0f };
		if (aiGetMaterialFloat(l_material, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, &lv_tmp) == AI_SUCCESS) {
			lv_outputMaterial.m_metallicFactor = lv_tmp;
		}
		if (aiGetMaterialFloat(l_material, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, &lv_tmp) == AI_SUCCESS) {
			lv_outputMaterial.m_roughness = { lv_tmp, lv_tmp, lv_tmp, lv_tmp };
		}

		aiString lv_path;
		std::string lv_newPath{};
		aiTextureMapping lv_textureMapping;
		unsigned int lv_uvIndex = 0;
		float lv_blend = 1.0f;
		aiTextureOp lv_textureOp = aiTextureOp_Add;
		aiTextureMapMode lv_textureMapMode[2] =
		{ aiTextureMapMode_Wrap, aiTextureMapMode_Wrap };
		unsigned int lv_textureFlags = 0;

		if (aiGetMaterialTexture(l_material, aiTextureType_EMISSIVE, 0, &lv_path, &lv_textureMapping, 
			&lv_uvIndex, &lv_blend, &lv_textureOp, lv_textureMapMode, &lv_textureFlags) == AI_SUCCESS) {
			lv_newPath = lv_path.C_Str();
			if (1 < lv_newPath.size()) {
				lv_newPath.erase(0, 2);
			}
			lv_outputMaterial.m_emissiveMap = AddStringToContainer(m_files, lv_newPath);

			if (-1 != lv_outputMaterial.m_emissiveMap) {
				lv_outputMaterial.m_flags |= (uint32_t)MaterialFlags::sMaterialFlags_EmissiveMapIncluded;
			}
		}

		if (aiGetMaterialTexture(l_material, aiTextureType_DIFFUSE, 0, &lv_path, &lv_textureMapping,
			&lv_uvIndex, &lv_blend, &lv_textureOp, lv_textureMapMode, &lv_textureFlags) == AI_SUCCESS) {
			lv_newPath = lv_path.C_Str();
			/*if (1 < lv_newPath.size()) {
				lv_newPath.erase(0, 2);
			}*/			
			lv_outputMaterial.m_albedoMap = AddStringToContainer(m_files, lv_newPath);

			if (-1 != lv_outputMaterial.m_albedoMap) {
				lv_outputMaterial.m_flags |= (uint32_t)MaterialFlags::sMaterialFlags_AlbedoMapIncluded;
			}
			else {
				printf("albedoMap index of -1 detected.\n");
			}
		}

		if (aiGetMaterialTexture(l_material, aiTextureType_NORMALS, 0, &lv_path, &lv_textureMapping,
			&lv_uvIndex, &lv_blend, &lv_textureOp, lv_textureMapMode, &lv_textureFlags) == AI_SUCCESS) {
			lv_newPath = lv_path.C_Str();
			/*if (1 < lv_newPath.size()) {
				lv_newPath.erase(0, 2);
			}*/
			lv_outputMaterial.m_normalMap = AddStringToContainer(m_files, lv_newPath);

			if (-1 != lv_outputMaterial.m_normalMap) {
				lv_outputMaterial.m_flags |= (uint32_t)MaterialFlags::sMaterialFlags_NormalMapIncluded;
			}
		}

		if (INVALID_TEXTURE == lv_outputMaterial.m_normalMap) {

			if (aiGetMaterialTexture(l_material, aiTextureType_HEIGHT, 0, &lv_path, &lv_textureMapping,
				&lv_uvIndex, &lv_blend, &lv_textureOp, lv_textureMapMode, &lv_textureFlags) == AI_SUCCESS) {
				lv_newPath = lv_path.C_Str();
				/*if (1 < lv_newPath.size()) {
					lv_newPath.erase(0, 2);
				}*/
				lv_outputMaterial.m_normalMap = AddStringToContainer(m_files, lv_newPath);
				if (-1 != lv_outputMaterial.m_normalMap) {
					lv_outputMaterial.m_flags |= (uint32_t)MaterialFlags::sMaterialFlags_NormalMapIncluded;
				}
			}
		}

		if (aiGetMaterialTexture(l_material, aiTextureType_OPACITY, 0, &lv_path, &lv_textureMapping,
			&lv_uvIndex, &lv_blend, &lv_textureOp, lv_textureMapMode, &lv_textureFlags) == AI_SUCCESS) {
			lv_newPath = lv_path.C_Str();
			/*if (1 < lv_newPath.size()) {
				lv_newPath.erase(0, 2);
			}*/
			lv_outputMaterial.m_opacityMap = AddStringToContainer(m_opaqueMapFiles, lv_newPath);
			lv_outputMaterial.m_alphaTest = 0.5f;

			if (-1 != lv_outputMaterial.m_opacityMap) {
				lv_outputMaterial.m_flags |= (uint32_t)MaterialFlags::sMaterialFlags_OpacityMapIncluded;
			}
		}

		if (aiGetMaterialTexture(l_material, aiTextureType_AMBIENT_OCCLUSION, 0, &lv_path, &lv_textureMapping,
			&lv_uvIndex, &lv_blend, &lv_textureOp, lv_textureMapMode, &lv_textureFlags) == AI_SUCCESS) {
			lv_newPath = lv_path.C_Str();
			/*if (1 < lv_newPath.size()) {
				lv_newPath.erase(0, 2);
			}*/
			lv_outputMaterial.m_ambientOcclusionMap = AddStringToContainer(m_files, lv_newPath);

			if (-1 != lv_outputMaterial.m_ambientOcclusionMap) {
				lv_outputMaterial.m_flags |= (uint32_t)MaterialFlags::sMaterialFlags_AmbientOcclusionMapIncluded;
			}
			else {
				printf("AoMap index of -1 detected.\n");
			}
		}

		if (aiGetMaterialTexture(l_material, aiTextureType_METALNESS, 0, &lv_path, &lv_textureMapping,
			&lv_uvIndex, &lv_blend, &lv_textureOp, lv_textureMapMode, &lv_textureFlags) == AI_SUCCESS) {
			lv_newPath = lv_path.C_Str();
			/*if (1 < lv_newPath.size()) {
				lv_newPath.erase(0, 2);
			}*/
			lv_outputMaterial.m_metallicMap = AddStringToContainer(m_files, lv_newPath);

			if (-1 != lv_outputMaterial.m_metallicMap) {
				lv_outputMaterial.m_flags |= (uint32_t)MaterialFlags::sMaterialFlags_MetallicMapIncluded;
			}
			else {
				printf("MetallicMap index of -1 detected.\n");
			}
		}


		if (aiGetMaterialTexture(l_material, aiTextureType_DIFFUSE_ROUGHNESS, 0, &lv_path, &lv_textureMapping,
			&lv_uvIndex, &lv_blend, &lv_textureOp, lv_textureMapMode, &lv_textureFlags) == AI_SUCCESS) {
			lv_newPath = lv_path.C_Str();
			/*if (1 < lv_newPath.size()) {
				lv_newPath.erase(0, 2);
			}*/
			lv_outputMaterial.m_roughnessMap = AddStringToContainer(m_files, lv_newPath);

			if (-1 != lv_outputMaterial.m_roughnessMap) {
				lv_outputMaterial.m_flags |= (uint32_t)MaterialFlags::sMaterialFlags_RoughnessMapIncluded;
			}
			else {
				printf("MetallicMap index of -1 detected.\n");
			}
		}

		m_materials.emplace_back(lv_outputMaterial);

		return lv_outputMaterial;

	}

	int MaterialLoaderAndSaver::AddStringToContainer(std::vector<std::string>& lv_stringContainer, 
		const std::string& lv_string)
	{
		int lv_indexOfFile{-1};

		if (true == lv_string.empty()) { return lv_indexOfFile; }

		auto lv_iteratorOfString = std::find(lv_stringContainer.begin(), lv_stringContainer.end(), lv_string);
		if (lv_iteratorOfString != lv_stringContainer.end()) {
			lv_indexOfFile = (int)std::distance(lv_stringContainer.begin(), lv_iteratorOfString);
			return lv_indexOfFile;
		}
		
		lv_indexOfFile = (int)lv_stringContainer.size();
		lv_stringContainer.push_back(lv_string);

		return lv_indexOfFile;
	}
}