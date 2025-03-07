



#include "PresentToColorAttachRenderer.hpp"



namespace RenderCore
{

	PresentToColorAttachRenderer::PresentToColorAttachRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator)
		:Renderbase(l_vkContextCreator)
	{

		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();

		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
			auto& lv_swapchainTexture = lv_vkResManager.RetrieveGpuTexture("Swapchain", i);
			transitionImageLayout(m_vulkanRenderContext.GetContextCreator().m_vkDev
									, lv_swapchainTexture.image.image, lv_swapchainTexture.format, lv_swapchainTexture.Layout
									, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
			lv_swapchainTexture.Layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		}

		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();
		auto* lv_node = lv_frameGraph.RetrieveNode("PresentToColorAttach");
		lv_frameGraph.IncrementNumNodesPerCmdBuffer(0);
		SetRenderPassAndFrameBuffer("PresentToColorAttach");
		SetNodeToAppropriateRenderpass("PresentToColorAttach", this);

	}

	void PresentToColorAttachRenderer::FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
		uint32_t l_currentSwapchainIndex)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		VkRect2D lv_screenRect{};
		lv_screenRect.extent = { .width = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferWidth,
			.height = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferHeight };
		lv_screenRect.offset = { 0, 0 };

		auto lv_framebuffer = lv_vkResManager.RetrieveGpuFramebuffer(m_framebufferHandles[l_currentSwapchainIndex]);

		m_vulkanRenderContext.BeginRenderPass(l_cmdBuffer, m_renderPass, l_currentSwapchainIndex,
			lv_screenRect, lv_framebuffer);
		vkCmdEndRenderPass(l_cmdBuffer);
	}

	void PresentToColorAttachRenderer::UpdateBuffers(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure)
	{

	}

	void PresentToColorAttachRenderer::UpdateDescriptorSets()
	{

	}


}