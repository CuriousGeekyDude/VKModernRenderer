

#include "VulkanClear.hpp"



namespace RenderCore
{
	VulkanClear::VulkanClear(VulkanEngine::VulkanRenderContext& l_vkContextCreator) : 
		Renderbase(l_vkContextCreator)
	{
		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();

		SetRenderPassAndFrameBuffer("ClearSwapchain");

		auto* lv_node = lv_frameGraph.RetrieveNode("ClearSwapchain");

		if (nullptr == lv_node) {
			printf("Indirect renderer was not found among the nodes of the frame graph. Exitting....\n");
			exit(-1);
		}
		lv_node->m_renderer = this;

		m_pipelineLayout = lv_vkResManager.CreatePipelineLayout(m_);


	}


	void VulkanClear::UpdateUniformBuffers(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure) {}

	void VulkanClear::UpdateDescriptorSets() {}


	void VulkanClear::FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
		uint32_t l_currentSwapchainIndex)
	{

		VkClearValue lv_clearValues[2] = {
			VkClearValue {.color = {1.0f, 1.0f, 1.0f, 1.0f} },
			VkClearValue {.depthStencil = { 1, 0 } }
		};
		
		VkRect2D lv_screenRect = {
				.offset = { 0, 0 },
				.extent = {.width = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferWidth,
						   .height = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferHeight }
		};

		m_vulkanRenderContext.BeginRenderPass(l_cmdBuffer, 
			m_vulkanRenderContext.GetClearRenderPass().m_renderpass, 
			l_currentSwapchainIndex, lv_screenRect, 
			m_vulkanRenderContext.GetSwapchainFramebufferDepth(l_currentSwapchainIndex), 
			2U, lv_clearValues);
		vkCmdEndRenderPass(l_cmdBuffer);
	}
}