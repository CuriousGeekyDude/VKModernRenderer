

#include "VulkanPresent.hpp"



namespace RenderCore
{
	VulkanPresent::VulkanPresent(VulkanEngine::VulkanRenderContext& l_vkContextCreator,
		VkFramebuffer l_frameBuffer, const RenderPass& l_renderpass) : 
		Renderbase(l_vkContextCreator, l_frameBuffer, l_renderpass) 
	{

	}


	void VulkanPresent::FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
		uint32_t l_currentSwapchainIndex, VkFramebuffer l_frameBuffer,
		VkRenderPass l_renderpass)
	{
		VkRect2D lv_screenRect = {
				.offset = { 0, 0 },
				.extent = {.width = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferWidth,
						   .height = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferHeight }
		};

		m_vulkanRenderContext.BeginRenderPass(l_cmdBuffer, 
			m_vulkanRenderContext.GetPresentRenderPass().m_renderpass, l_currentSwapchainIndex, 
			lv_screenRect, m_vulkanRenderContext.GetSwapchainFramebufferDepth(l_currentSwapchainIndex));
		vkCmdEndRenderPass(l_cmdBuffer);
	}
}