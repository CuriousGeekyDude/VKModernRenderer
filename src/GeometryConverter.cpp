


#include "GeometryConverter.hpp"
#include "meshoptimizer.h"
#include <array>

namespace MeshConverter
{


	void GeometryConverter::CalculateBoundingBox(const uint32_t l_meshIndex, BoundingBox& l_box,
												const aiMesh* l_mesh)
	{
		l_box.m_instanceDataIndex = l_meshIndex;

		l_box.m_min = glm::vec4{l_mesh->mAABB.mMin.x *0.05, l_mesh->mAABB.mMin.y*0.05, l_mesh->mAABB.mMin.z*0.05, 1.f};
		l_box.m_max = glm::vec4{l_mesh->mAABB.mMax.x * 0.05, l_mesh->mAABB.mMax.y * 0.05, l_mesh->mAABB.mMax.z * 0.05, 1.f };


	}

	void GeometryConverter::ConvertScene(const std::string& l_sceneFileName, const std::string& l_meshFileHeaders,
		const std::string& l_boundingBoxFile,
		const std::string& l_instanceDataFile,
		bool l_includeTextureCoordinates,
		bool l_includeNormals,
		bool l_tangents)
	{
		m_meshes.clear();
		m_indexBuffer.clear();
		m_vertexBuffer.clear();
		m_outputInstanceData.clear();
		m_boundingBoxes.clear();
		m_includeNormals = l_includeNormals;
		m_includeTextureCoordinates = l_includeTextureCoordinates;
		m_includeTangents = l_tangents;
		m_totalNumIndicesScene = 0;
		m_totalNumVerticesScene = 0;
		m_assimpScene = nullptr;
		m_sceneConverter.reset();
		

		const unsigned int lv_postProcessFlags = aiProcess_JoinIdenticalVertices |
			aiProcess_Triangulate |
			aiProcess_GenSmoothNormals |
			aiProcess_LimitBoneWeights |
			aiProcess_CalcTangentSpace |
			aiProcess_SplitLargeMeshes |
			aiProcess_ImproveCacheLocality |
			aiProcess_RemoveRedundantMaterials |
			aiProcess_FindDegenerates |
			aiProcess_FindInvalidData |
			aiProcess_ValidateDataStructure |
			aiProcess_GenBoundingBoxes |
			aiProcess_GenUVCoords;

		printf("Conversion of file %s has begun...", l_sceneFileName.c_str());

		

		const aiScene* lv_scene = m_importer.ReadFile(l_sceneFileName, lv_postProcessFlags);



		if (nullptr == lv_scene) {
			printf("Assimp failed to import scene.\n");
			printf("Error: %s\n", m_importer.GetErrorString());
			exit(EXIT_FAILURE);
		}

		m_assimpScene = lv_scene;
		m_sceneConverter.emplace(m_assimpScene);

		if (false == lv_scene->HasMeshes()) {
			printf("The loaded scene has no meshes.");
			exit(EXIT_FAILURE);
		}


		CalculateTotalNumVerticesAndIndicesOfScene(lv_scene);

		uint32_t lv_numElementsVertexBuffer = 3;
		if (true == m_includeTextureCoordinates) { lv_numElementsVertexBuffer += 2; }
		if (true == m_includeNormals) { lv_numElementsVertexBuffer += 3; }
		if (true == m_includeTangents) { lv_numElementsVertexBuffer += 4; }

		m_meshes.reserve(lv_scene->mNumMeshes);
		m_vertexBuffer.resize(m_totalNumVerticesScene * lv_numElementsVertexBuffer);
		m_indexBuffer.resize(m_totalNumIndicesScene);
		m_boundingBoxes.resize(lv_scene->mNumMeshes);

		uint32_t lv_vertexOffset{};
		uint32_t lv_indexOffset{};
		for (uint32_t i = 0; i < lv_scene->mNumMeshes; ++i) {
			ConvertMesh(lv_scene->mMeshes[i], lv_vertexOffset, lv_indexOffset);
			CalculateBoundingBox(i, m_boundingBoxes[i], lv_scene->mMeshes[i]);
			lv_vertexOffset += lv_scene->mMeshes[i]->mNumVertices*lv_numElementsVertexBuffer;
			lv_indexOffset += lv_scene->mMeshes[i]->mNumFaces * 3 * lv_maxLODCount-1;
		}




		MeshFileHeader lv_fileHeader{};
		lv_fileHeader.m_magicValue = 0X12345678;
		lv_fileHeader.m_lodDataSize = m_totalNumIndicesScene * sizeof(unsigned int);
		lv_fileHeader.m_meshCount = lv_scene->mNumMeshes;
		lv_fileHeader.m_vertexDataSize = m_totalNumVerticesScene * lv_numElementsVertexBuffer * sizeof(float);
		lv_fileHeader.m_startBlockOffset = (uint32_t)(sizeof(MeshFileHeader) + sizeof(Mesh)*m_meshes.size());

		SaveDataToMeshFileHeader(l_meshFileHeaders, lv_fileHeader);
		printf("\n\nMesh data was successfully generated and saved.\n\n");

		SaveBoundingBoxData(l_boundingBoxFile);

		printf("\n\nBounding box data was successfully generated and saved.\n\n");

		printf("\n\nGenerating instance file now...");
		GenerateInstanceDataFile_TextureCoordNormalsTangents(l_instanceDataFile);
		printf("\nInstance file generation was completed successfully.\n");

	}

	void GeometryConverter::SaveDataToMeshFileHeader(const std::string& l_meshFileHeader, 
		MeshFileHeader& l_meshFileHeaderStructure)
	{
		FILE* lv_meshFileHeader = fopen(l_meshFileHeader.c_str(), "wb");

		if (nullptr == lv_meshFileHeader) {
			printf("Mesh file header failed to open.\n");
			exit(EXIT_FAILURE);
		}

		fwrite(&l_meshFileHeaderStructure, sizeof(MeshFileHeader), 1, lv_meshFileHeader);
		fwrite(m_meshes.data(), sizeof(Mesh), m_meshes.size(), lv_meshFileHeader);
		fwrite(m_vertexBuffer.data(), sizeof(float), m_vertexBuffer.size(), lv_meshFileHeader);
		fwrite(m_indexBuffer.data(), sizeof(unsigned int), m_indexBuffer.size(), lv_meshFileHeader);
		fclose(lv_meshFileHeader);
	}


	void GeometryConverter::CalculateTotalNumVerticesAndIndicesOfScene(const aiScene* l_scene)
	{
		uint32_t lv_totalNumMeshes{l_scene->mNumMeshes};

		for (uint32_t i = 0; i < lv_totalNumMeshes; ++i) {
			m_totalNumVerticesScene += l_scene->mMeshes[i]->mNumVertices;
			m_totalNumIndicesScene += l_scene->mMeshes[i]->mNumFaces * 3 * lv_maxLODCount-1;
		}

	}



	void GeometryConverter::GenerateMeshLODs(std::span<unsigned int> l_originalIndices,
		std::span<float> l_meshVertices,
		uint32_t l_totalNumStreams,
		std::array<std::vector<unsigned int>, lv_maxLODCount-1>& l_lods)
	{
		std::vector<unsigned int> lv_tempIndices{};
		uint32_t lv_lodCount{};

		lv_tempIndices.resize(l_originalIndices.size());
		for (uint32_t i = 0; i < lv_tempIndices.size(); ++i) {
			lv_tempIndices[i] = l_originalIndices[i];
		}

		

		l_lods[0] = lv_tempIndices;
		++lv_lodCount;


		while (lv_lodCount < lv_maxLODCount-1) {

			auto lv_numReducedIndices = meshopt_simplify(lv_tempIndices.data(), lv_tempIndices.data()
				, lv_tempIndices.size(), l_meshVertices.data(), l_meshVertices.size()/ l_totalNumStreams,
				sizeof(float)*l_totalNumStreams, lv_tempIndices.size() * 0.5, 0, nullptr);


			if (lv_numReducedIndices * 1.1f > lv_tempIndices.size()) {

				lv_numReducedIndices = meshopt_simplifySloppy(lv_tempIndices.data(), lv_tempIndices.data()
					, lv_tempIndices.size(), l_meshVertices.data(), l_meshVertices.size() / l_totalNumStreams,
					sizeof(float) * l_totalNumStreams, lv_tempIndices.size() * 0.5, 0, nullptr);

			}


			lv_tempIndices.resize(lv_numReducedIndices);

			meshopt_optimizeVertexCache(lv_tempIndices.data(), lv_tempIndices.data(),
				lv_tempIndices.size(), l_meshVertices.size() / l_totalNumStreams);
			meshopt_optimizeOverdraw(lv_tempIndices.data(), lv_tempIndices.data()
				, lv_tempIndices.size(), l_meshVertices.data(), l_meshVertices.size() / l_totalNumStreams,
				sizeof(float) * l_totalNumStreams, 1.05f);
			

			l_lods[lv_lodCount] = lv_tempIndices;
			++lv_lodCount;
		}

		
	}


	void GeometryConverter::ConvertMesh(const aiMesh* l_mesh, uint32_t& l_vertexOffset,
		uint32_t& l_indexOffset)
	{

		printf("\n----------------------\n");
		printf("\nConverting mesh %s\n\n", l_mesh->mName.C_Str());

		uint32_t lv_numIndices = l_mesh->mNumFaces * 3;
		uint32_t lv_numElementsVertexBuffer = 3;
		uint32_t lv_streamCount = 1;

		if (true == m_includeTextureCoordinates) { 
			lv_numElementsVertexBuffer += 2; 
			++lv_streamCount;
		}
		if (true == m_includeNormals) { 
			lv_numElementsVertexBuffer += 3; 
			++lv_streamCount;
		}

		if (true == m_includeTangents) {
			lv_numElementsVertexBuffer += 4;
			++lv_streamCount;
		}

		
		const bool lv_includeTexCoordsOnly = m_includeTextureCoordinates && !(m_includeNormals || m_includeTangents);
		const bool lv_includeTexCoordNormalsOnly = (m_includeTextureCoordinates && m_includeNormals) && !m_includeTangents;

		for (uint32_t i = 0, j = 0; 
			i < l_mesh->mNumVertices*lv_numElementsVertexBuffer && j < l_mesh->mNumVertices; 
			i += lv_numElementsVertexBuffer, ++j) {
			const aiVector3D& lv_pos = l_mesh->mVertices[j];

			m_vertexBuffer[l_vertexOffset+i] = ((float)lv_pos.x * 0.05f);
			m_vertexBuffer[l_vertexOffset + i + 1] = ((float)lv_pos.y * 0.05f);
			m_vertexBuffer[l_vertexOffset + i + 2] = ((float)lv_pos.z * 0.05f);


			if (true == lv_includeTexCoordsOnly) {
				if (true == l_mesh->HasTextureCoords(0)) {
					const aiVector3D& lv_uv = l_mesh->mTextureCoords[0][j];

					m_vertexBuffer[l_vertexOffset + i + 3] = ((float)lv_uv.x);
					m_vertexBuffer[l_vertexOffset + i + 4] = ((float)lv_uv.y);
				}
				else {
					m_vertexBuffer[l_vertexOffset + i + 3] = (1.f);
					m_vertexBuffer[l_vertexOffset + i + 4] = (1.f);
				}
			}

			else if (true == lv_includeTexCoordNormalsOnly) {

				if (true == l_mesh->HasTextureCoords(0)) {
					const aiVector3D& lv_uv = l_mesh->mTextureCoords[0][j];

					m_vertexBuffer[l_vertexOffset + i + 3] = ((float)lv_uv.x);
					m_vertexBuffer[l_vertexOffset + i + 4] = ((float)lv_uv.y);
				}
				else {
					m_vertexBuffer[l_vertexOffset + i + 3] = (1.f);
					m_vertexBuffer[l_vertexOffset + i + 4] = (1.f);
				}

				if (true == l_mesh->HasNormals()) {
					const aiVector3D& lv_normal = l_mesh->mNormals[j];

					m_vertexBuffer[l_vertexOffset + i + 5] = ((float)lv_normal.x);
					m_vertexBuffer[l_vertexOffset + i + 6] = ((float)lv_normal.y);
					m_vertexBuffer[l_vertexOffset + i + 7] = ((float)lv_normal.z);
				}

				else {
					m_vertexBuffer[l_vertexOffset + i + 5] = (1.f);
					m_vertexBuffer[l_vertexOffset + i + 6] = (1.f);
					m_vertexBuffer[l_vertexOffset + i + 7] = (1.f);
				}

			}

			else {


				if (true == l_mesh->HasTextureCoords(0)) {
					const aiVector3D& lv_uv = l_mesh->mTextureCoords[0][j];

					m_vertexBuffer[l_vertexOffset + i + 3] = ((float)lv_uv.x);
					m_vertexBuffer[l_vertexOffset + i + 4] = ((float)lv_uv.y);
				}
				else {
					m_vertexBuffer[l_vertexOffset + i + 3] = (1.f);
					m_vertexBuffer[l_vertexOffset + i + 4] = (1.f);
				}



				if (true == l_mesh->HasNormals()) {
					const aiVector3D& lv_normal = l_mesh->mNormals[j];

					m_vertexBuffer[l_vertexOffset + i + 5] = ((float)lv_normal.x);
					m_vertexBuffer[l_vertexOffset + i + 6] = ((float)lv_normal.y);
					m_vertexBuffer[l_vertexOffset + i + 7] = ((float)lv_normal.z);
				}
				else {
					m_vertexBuffer[l_vertexOffset + i + 5] = (1.f);
					m_vertexBuffer[l_vertexOffset + i + 6] = (1.f);
					m_vertexBuffer[l_vertexOffset + i + 7] = (1.f);
				}



				if (true == l_mesh->HasTangentsAndBitangents()) {
					
					const aiVector3D& lv_tangent = l_mesh->mTangents[j];
					const auto& lv_bitangent = l_mesh->mBitangents[j];

					m_vertexBuffer[l_vertexOffset + i + 8] = (float)lv_tangent.x;
					m_vertexBuffer[l_vertexOffset + i + 9] = (float)lv_tangent.y;
					m_vertexBuffer[l_vertexOffset + i + 10] = (float)lv_tangent.z;
					m_vertexBuffer[l_vertexOffset + i + 11] = (float)lv_bitangent.x;

				}
				else {
					m_vertexBuffer[l_vertexOffset + i + 8] = 1.f;
					m_vertexBuffer[l_vertexOffset + i + 9] = 1.f;
					m_vertexBuffer[l_vertexOffset + i + 10] = 1.f;
					m_vertexBuffer[l_vertexOffset + i + 11] = 1.f;
				}

			}

		}

		uint32_t lv_realNumIndices{};
		printf("\nl_indexOffset: %d\n", l_indexOffset);
		for (uint32_t i = 0, j = 0; i < l_mesh->mNumFaces && j < l_mesh->mNumFaces*3; ++i, j+=3) {

			if (l_mesh->mFaces[i].mNumIndices == 3) {
				m_indexBuffer[l_indexOffset + j] = (l_mesh->mFaces[i].mIndices[0]);
				m_indexBuffer[l_indexOffset + j + 1] = (l_mesh->mFaces[i].mIndices[1]);
				m_indexBuffer[l_indexOffset + j + 2] = (l_mesh->mFaces[i].mIndices[2]);
				lv_realNumIndices += 3U;
			}
		}


		
		std::span<unsigned int> lv_meshIndices{&m_indexBuffer[l_indexOffset], lv_realNumIndices};
		std::span<float> lv_meshVertices{&m_vertexBuffer[l_vertexOffset], l_mesh->mNumVertices*lv_numElementsVertexBuffer};
		std::array<std::vector<unsigned int>, lv_maxLODCount-1> lv_meshLOD{};

		GenerateMeshLODs(lv_meshIndices, lv_meshVertices, lv_numElementsVertexBuffer, lv_meshLOD);


		uint32_t lv_indexCount{l_indexOffset + lv_realNumIndices};
		for (uint32_t j = 1; j < lv_maxLODCount - 1; ++j) {
			printf("LOD size: %u\n", (uint32_t)lv_meshLOD[j].size());
			for (uint32_t i = 0; i < lv_meshLOD[j].size(); ++i) {
				m_indexBuffer[lv_indexCount + i] = (lv_meshLOD[j][i]);
			}
			lv_indexCount += lv_meshLOD[j].size();
		}

		printf("\nLOD of this mesh was processed and copied successfully.\n");
		printf("Total num of indices of the scene: %u\n\n", m_totalNumIndicesScene);


		Mesh lv_mesh{};
		lv_mesh.m_lodCount = lv_maxLODCount-1;

		uint32_t lv_lodOffset{ l_indexOffset * sizeof(unsigned int) };
		for (uint32_t i = 0; i < lv_maxLODCount; ++i) {
			if (i < lv_maxLODCount - 1) {
				lv_mesh.m_lodOffsets[i] = lv_lodOffset;
				lv_lodOffset += lv_meshLOD[i].size() * sizeof(unsigned int);
			}
			else {
				lv_mesh.m_lodOffsets[lv_maxLODCount - 1] = lv_lodOffset;
			}
		}

		lv_mesh.m_meshSize = (uint32_t)(sizeof(float) * lv_numElementsVertexBuffer * l_mesh->mNumVertices + lv_numIndices*sizeof(unsigned int));
		lv_mesh.m_streamCount = lv_streamCount;
		lv_mesh.m_streamElementSizes[0] = 3 * sizeof(float);
		lv_mesh.m_streamOffsets[0] = l_vertexOffset*sizeof(float);
		lv_mesh.m_vertexCount = l_mesh->mNumVertices;
		
		if (true == lv_includeTexCoordsOnly) { 
			lv_mesh.m_streamElementSizes[1] = 2 * sizeof(float);
			lv_mesh.m_streamOffsets[1] = (l_vertexOffset + 3)*sizeof(float);
		}
		else if (true == lv_includeTexCoordNormalsOnly) { 

			lv_mesh.m_streamElementSizes[1] = 2 * sizeof(float);
			lv_mesh.m_streamOffsets[1] = (l_vertexOffset + 3) * sizeof(float);

			lv_mesh.m_streamElementSizes[2] = 3 * sizeof(float);
			lv_mesh.m_streamOffsets[2] = lv_mesh.m_streamOffsets[1] + 2 * sizeof(float);
		}
		else {
			lv_mesh.m_streamElementSizes[1] = 2 * sizeof(float);
			lv_mesh.m_streamOffsets[1] = (l_vertexOffset + 3) * sizeof(float);

			lv_mesh.m_streamElementSizes[2] = 3 * sizeof(float);
			lv_mesh.m_streamOffsets[2] = lv_mesh.m_streamOffsets[1] + 2 * sizeof(float);

			lv_mesh.m_streamElementSizes[3] = 4 * sizeof(float);
			lv_mesh.m_streamOffsets[3] = lv_mesh.m_streamOffsets[2] + 3 * sizeof(float);
		}


		m_meshes.emplace_back(lv_mesh);

		

	}


	void GeometryConverter::SaveBoundingBoxData(const std::string& l_boundingBoxFile)
	{
		FILE* lv_boundingBoxFile = fopen(l_boundingBoxFile.c_str(), "wb");

		if (nullptr == lv_boundingBoxFile) {
			printf("Failed to open bounding box file for writing.\n");
			exit(EXIT_FAILURE);
		}

		uint32_t lv_boundingBoxSize = m_boundingBoxes.size();


		if (1 != fwrite(&lv_boundingBoxSize, sizeof(uint32_t), 1, lv_boundingBoxFile)) {
			printf("Failed to write total number of bounding boxes to the bounding box file.\n");
			exit(EXIT_FAILURE);
		}


		if (lv_boundingBoxSize * sizeof(MeshConverter::GeometryConverter::BoundingBox) != fwrite(m_boundingBoxes.data(), 1, lv_boundingBoxSize * sizeof(MeshConverter::GeometryConverter::BoundingBox), lv_boundingBoxFile)) {
			printf("Failed to write bounding box data to bounding box file.\n");
			exit(EXIT_FAILURE);
		}


	}

	void GeometryConverter::GenerateInstanceDataFile_TextureCoordNormalsTangents(const std::string& l_instanceDataFile)
	{
		using namespace RenderCore;
		FILE* lv_instanceDataFile = fopen(l_instanceDataFile.c_str(), "wb");

		if (nullptr == lv_instanceDataFile) {
			printf("Instance file failed to open for writing.\n");
			exit(EXIT_FAILURE);
		}

		m_outputInstanceData.resize(m_meshes.size());

		for (uint32_t i = 0; i < m_outputInstanceData.size(); ++i) {
			InstanceData lv_instanceData{};
			lv_instanceData.m_materialIndex = m_assimpScene->mMeshes[i]->mMaterialIndex;
			lv_instanceData.m_lod = 0;
			lv_instanceData.m_indexBufferIndex = m_meshes[i].m_lodOffsets[0]/sizeof(unsigned int);
			lv_instanceData.m_meshIndex = i;
			lv_instanceData.m_vertexBufferIndex = m_meshes[i].m_streamOffsets[0] / (12*sizeof(float));


			for (auto& l_nodeMeshTuple : m_sceneConverter.value().GetScene().m_meshes) {
				if (i == l_nodeMeshTuple.second) {
					lv_instanceData.m_transformIndex = l_nodeMeshTuple.first;
					break;
				}
			}

			m_outputInstanceData[i] = (lv_instanceData);
		}

		auto lv_totalNumInstances = (uint32_t)m_outputInstanceData.size();

		if (1 != fwrite(&lv_totalNumInstances, sizeof(uint32_t), 1, lv_instanceDataFile)) {
			printf("\nFailed to write the total number of instaces to the instance data file.\n");
			exit(EXIT_FAILURE);
		}

		if (m_outputInstanceData.size() != fwrite(m_outputInstanceData.data(), sizeof(InstanceData), m_outputInstanceData.size(), lv_instanceDataFile)) {
			printf("\nFailed to write to the instance data file\n");
			exit(EXIT_FAILURE);
		}

		fclose(lv_instanceDataFile);
	}
}


