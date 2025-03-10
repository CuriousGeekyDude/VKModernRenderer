



#include "ClearSwapchainDepthRenderer.hpp"



namespace RenderCore
{

	ClearSwapchainDepthRenderer::ClearSwapchainDepthRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator)
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

		m_swapchainTextures.resize(lv_totalNumSwapchains);
		m_depthTextures.resize(lv_totalNumSwapchains);

		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
			m_swapchainTextures[i] = &lv_vkResManager.RetrieveGpuTexture("Swapchain", i);
			m_depthTextures[i] = &lv_vkResManager.RetrieveGpuTexture("Depth", i);
		}

		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();
		lv_frameGraph.IncrementNumNodesPerCmdBuffer(0);
		SetRenderPassAndFrameBuffer("ClearSwapchainDepth");
		SetNodeToAppropriateRenderpass("ClearSwapchainDepth", this);
	}

	void ClearSwapchainDepthRenderer::FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
		uint32_t l_currentSwapchainIndex)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_framebuffer = lv_vkResManager.RetrieveGpuFramebuffer(m_framebufferHandles[l_currentSwapchainIndex]);


		transitionImageLayoutCmd(l_cmdBuffer, m_swapchainTextures[l_currentSwapchainIndex]->image.image
								, m_swapchainTextures[l_currentSwapchainIndex]->format
								, m_swapchainTextures[l_currentSwapchainIndex]->Layout
								, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		transitionImageLayoutCmd(l_cmdBuffer, m_depthTextures[l_currentSwapchainIndex]->image.image
			, m_depthTextures[l_currentSwapchainIndex]->format
			, m_depthTextures[l_currentSwapchainIndex]->Layout
			, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		m_swapchainTextures[l_currentSwapchainIndex]->Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		m_depthTextures[l_currentSwapchainIndex]->Layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


		VkRect2D lv_screenRect{};
		lv_screenRect.extent = { .width = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferWidth,
			.height = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferHeight };
		lv_screenRect.offset = { 0, 0 };

		VkClearValue lv_clearValues[2] = {
		VkClearValue {.color = {1.f, 1.f, 1.f, 1.f} },
		VkClearValue {.depthStencil = {1.f, 0} }
		};



		m_vulkanRenderContext.BeginRenderPass(l_cmdBuffer, m_renderPass, l_currentSwapchainIndex, lv_screenRect,
			lv_framebuffer, 2, lv_clearValues);
		vkCmdEndRenderPass(l_cmdBuffer);


	}

	void ClearSwapchainDepthRenderer::UpdateBuffers(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure)
	{

	}

	void ClearSwapchainDepthRenderer::UpdateDescriptorSets()
	{

	}

}