#pragma once


#include "Renderbase.hpp"
#include <glm/glm.hpp>

namespace RenderCore
{
	class VulkanCubeRenderer : public Renderbase
	{
	public:

		VulkanCubeRenderer(VulkanRenderDevice& l_renderDevice, VulkanImage l_depth, 
			const char* l_textureFile);

		virtual ~VulkanCubeRenderer();

		virtual void FillCommandBuffer(VkCommandBuffer l_commandBuffer,
			uint32_t l_currentSwapchainIndex, VkFramebuffer l_frameBuffer = VK_NULL_HANDLE,
			VkRenderPass l_renderpass = VK_NULL_HANDLE) override;

		void UpdateStorageBuffers(VulkanRenderDevice& l_renderDevice, 
			uint32_t l_currentSwapchainIndex, const glm::mat4& l_mvp);


	private:

		bool CreateDescriptorSet(VulkanRenderDevice& l_renderDevice);


	private:
		VkSampler m_cubeTextureSampler;
		VulkanImage m_cubeTexture;

	};
}


