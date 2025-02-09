#pragma once


#include "UtilsVulkan.h"
#include "InstanceData.hpp"
#include "Renderbase.hpp"
#include "Mesh.hpp"
#include "MeshFileHeader.hpp"
#include <vector>
#include <glm/glm.hpp>
#include "GeometryConverter.hpp"
#include "MaterialLoaderAndSaver.hpp"
#include "SceneLoaderAndSaver.hpp"



namespace VulkanEngine
{
	struct CameraStructure;
}



namespace RenderCore
{
	class IndirectRenderer : public Renderbase
	{
		typedef RenderCore::VulkanResourceManager::RenderPass RenderPass;

		struct IndirectUniformBuffer
		{

			glm::mat4 m_mvp;
			glm::vec4 m_cameraPos;


			float scale;
			float bias;
			float zNear;
			float zFar;
			float radius;
			float attScale;
			float distScale;
			uint32_t m_enableDeferred;

		};

	public:

		IndirectRenderer(VulkanEngine::VulkanRenderContext& l_vkRenderContext,
			VkCommandBuffer l_cmdBuffer,
			const char* l_meshHeaderFile,
			const char* l_boundingBoxFile,
			const char* l_instanceDataFile,
			const char* l_materialFile,
			const char* l_sceneFile,
			const char* l_vtxShaderFile,
			const char* l_fragShaderFile,
			const std::string& l_spirvFile);


		
		virtual void UpdateStorageBuffers(uint32_t l_currentSwapchainIndex);



		virtual void UpdateBuffers(const uint32_t l_currentSwapchainIndex,
			const VulkanEngine::CameraStructure& l_cameraStructure) override;


		virtual void FillCommandBuffer(
			VkCommandBuffer l_commandBuffer,
			uint32_t l_currentSwapchainIndex) override;

	protected:

		void UpdateGeometryBuffers(VkCommandBuffer l_cmdBuffer);
		void UpdateIndirectBuffer(uint32_t l_currentSwapchainIndex);
		void UpdateInstanceBuffer(uint32_t l_currentSwapchainIndex);
		void UpdateTransformationsBuffer(uint32_t l_currentSwapchainIndex);
		void UpdateLocalDeviceBuffers(VkCommandBuffer l_cmdBuffer, 
			const uint32_t l_bufferHandle, const void* l_bufferDataToTransfer);
		/*void UpdateLocalDeviceTextures(VkCommandBuffer l_cmdBuffer,
			const std::vector<VulkanTexture>& l_texturesToTransfer);*/



		//virtual void CreateRenderPass() override;
		virtual void UpdateDescriptorSets() override;
		//virtual void CreateFramebuffers() override;

		void LoadAllTexturesOfScene();

		void LoadInstanceData(const char* l_instanceFile);
		MeshConverter::MeshFileHeader LoadMeshData(const char* l_meshFileHeader);
		MeshConverter::GeometryConverter::BoundingBox LoadBoundingBoxData(const char* l_boundingBoxFile);

	private:


		MaterialLoaderAndSaver::MaterialLoaderAndSaver m_materialLoaderSaver;
		SceneLoaderAndSaver::SceneLoaderAndSaver m_sceneLoaderSaver;




		std::vector<InstanceData> m_outputInstanceData{};
		std::vector<MeshConverter::Mesh> m_meshes{};
		std::vector<uint32_t> m_indexBuffers{};
		std::vector<float> m_vertexBuffers{};
		std::vector<MeshConverter::GeometryConverter::BoundingBox> m_boundingBoxes;

		uint32_t m_vertexBufferSize;
		uint32_t m_indexBufferSize;
		uint32_t m_totalNumInstances;
		uint32_t m_instanceBufferSize;
		uint32_t m_materialBufferSize;


		uint32_t m_vertexDataBufferHandle{};
		uint32_t m_indicesDataBufferHandle{};
		uint32_t m_materialBufferHandle{};
		std::vector<uint32_t> m_arrayTexturesHandles{};

		std::vector<uint32_t> m_transformationsBufferHandles{};
		std::vector<uint32_t> m_instanceBufferHandles{};
		std::vector<uint32_t> m_indirectBufferHandles{};


		std::vector<uint32_t> m_attachmentHandles;
		std::vector<uint32_t> m_textureHandlesOfScene;
	};
}