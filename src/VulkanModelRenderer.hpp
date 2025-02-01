#pragma once 



#include "Renderbase.hpp"


namespace RenderCore
{
	class VulkanModelRenderer : public Renderbase
	{

	public:

		VulkanModelRenderer(VulkanRenderDevice& l_renderDevice,
			VulkanImage l_depth,
			const char* l_modelFile,
			const char* l_textureFile,
			uint32_t l_uniformDataSize);


		void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex, VkFramebuffer l_frameBuffer = VK_NULL_HANDLE,
			VkRenderPass l_renderpass = VK_NULL_HANDLE) override;


		void UpdateStorageBuffers(VulkanRenderDevice& l_renderDevice,
			uint32_t l_currentSwapchainIndex,
			const void* l_data, size_t l_dataSize);

		bool CreateDescriptorSet( VulkanRenderDevice& l_renderDevice,
			uint32_t l_uniformBufferDataSize);

		virtual ~VulkanModelRenderer();

	private:

		size_t m_vertexSubArraySize{};
		size_t m_indiceSubArraySize{};

		VkBuffer m_storageVertexIndexBuffer{};
		VkDeviceMemory m_storageVertexIndexGpuMemory{};

		VkSampler m_textureSampler{};
		VulkanImage m_modelTexture{};
	};
}