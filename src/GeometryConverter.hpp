#pragma once 


#include "Mesh.hpp"
#include "MeshFileHeader.hpp"
#include "InstanceData.hpp"
#include <vector>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <string>
#include <span>
#include <glm/glm.hpp>
#include <limits>
#include "SceneConverter.hpp"
#include <optional>

namespace MeshConverter
{

	class GeometryConverter final
	{
	public:

		struct BoundingBox final
		{
			glm::vec4 m_min{ std::numeric_limits<float>::max() };
			glm::vec4 m_max{ std::numeric_limits<float>::min() };
		};


	public:

		void ConvertScene(const std::string& l_sceneFileName, const std::string& l_meshFileHeader,
			const std::string& l_boundingBoxFile,
			const std::string& l_instanceDataFile,
			bool l_includeTextureCoordinates,
			bool l_includeNormals,
			bool l_tangents);


		const aiScene* GetCurrentaiScene() const { return m_assimpScene; }


		SceneConverter::Scene& GetScene() { assert(true == m_sceneConverter.has_value()); return m_sceneConverter.value().GetScene(); }

	private:

		void CalculateBoundingBox(const MeshConverter::Mesh&, BoundingBox& l_box);

		void GenerateMeshLODs(std::span<unsigned int> l_originalIndices, std::span<float> l_meshVertices, 
			uint32_t l_totalNumStreams,
			std::array<std::vector<unsigned int>, lv_maxLODCount-1>& l_lods);

		void ConvertMesh(const aiMesh* l_mesh, uint32_t& l_vertexOffset, 
			uint32_t& l_indexOffset);

		void CalculateTotalNumVerticesAndIndicesOfScene(const aiScene* l_scene);

		void SaveDataToMeshFileHeader(const std::string& l_meshFileHeader, 
			MeshFileHeader& l_meshFileHeaderStructure);

		void SaveBoundingBoxData(const std::string& l_boundingBoxFile);

		void GenerateInstanceDataFile_TextureCoordNormalsTangents(const std::string& l_instanceDataFile);

	private:

		std::vector<Mesh> m_meshes{};
		std::vector<float> m_vertexBuffer{};
		std::vector<unsigned int> m_indexBuffer{};
		std::vector<BoundingBox> m_boundingBoxes{};
		std::vector<RenderCore::InstanceData> m_outputInstanceData{};
		bool m_includeTextureCoordinates{false};
		bool m_includeNormals{ false };
		bool m_includeTangents{ false };
		uint32_t m_totalNumVerticesScene{};
		uint32_t m_totalNumIndicesScene{};
		const aiScene* m_assimpScene;
		Assimp::Importer m_importer;
		std::optional<SceneConverter::SceneConverter> m_sceneConverter;
	};
}