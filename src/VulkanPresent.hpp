#pragma once



#include "Renderbase.hpp"


namespace RenderCore
{

	class VulkanPresent : public Renderbase
	{
		typedef RenderCore::VulkanResourceManager::RenderPass RenderPass;

	public:

		VulkanPresent(VulkanEngine::VulkanRenderContext& l_vkContextCreator,
			VkFramebuffer l_frameBuffer,
			const RenderPass& l_renderpass);

		void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex, VkFramebuffer l_frameBuffer = VK_NULL_HANDLE,
			VkRenderPass l_renderpass = VK_NULL_HANDLE) override;

	};


}