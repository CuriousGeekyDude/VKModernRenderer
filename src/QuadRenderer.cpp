


#include "QuadRenderer.hpp"
#include <format>


namespace RenderCore
{
	QuadRenderer::QuadRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator,
		const std::string& l_vulkanDebugNameRenderer,
		const RenderCore::VulkanResourceManager::RenderPass& l_renderpass,
		const RenderCore::VulkanResourceManager::PipelineInfo& l_pipelineInfo,
		const std::vector<VkFramebuffer>& l_frameBuffers,
		std::vector<VulkanTexture>* l_frameBufferTextures,
		const std::vector<RenderCore::VulkanResourceManager::DescriptorSetResources>& l_descriptorSetInfos,
		const std::vector<const char*>& l_shaders)
		:Renderbase(l_vkContextCreator, l_renderpass),
		m_frameBufferTextures(l_frameBufferTextures)
	{
		auto& lv_vulkanResourceManager = m_vulkanRenderContext.GetResourceManager();
		

		m_descriptorSetLayout = lv_vulkanResourceManager.CreateDescriptorSetLayout(l_descriptorSetInfos[0],
			std::format(" Descriptor-Set-Layout-Quad ").c_str());
		m_descriptorPool = lv_vulkanResourceManager.CreateDescriptorPool(l_descriptorSetInfos[0],
			m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size()
			,std::format(" Descriptor-Pool-Quad ").c_str());
		

		for (uint32_t i = 0; i < m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size(); ++i) {
			m_descriptorSets.push_back(lv_vulkanResourceManager.CreateDescriptorSet(m_descriptorPool, 
				m_descriptorSetLayout, std::format(" Descriptor-Set-Quad {} ", i).c_str()));
		}

		for (size_t i = 0; i < l_vkContextCreator.GetContextCreator().m_vkDev.m_swapchainImages.size(); ++i) {
			lv_vulkanResourceManager.UpdateDescriptorSet(l_descriptorSetInfos[i], m_descriptorSets[i]);
		}
		

		m_framebufferHandles = l_frameBuffers;

		InitializeGraphicsPipeline(l_shaders, l_pipelineInfo);

	}


	void QuadRenderer::FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
		uint32_t l_currentSwapchainIndex)
	{
		
		
		if (nullptr != m_frameBufferTextures) {
			if (VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL == (*m_frameBufferTextures)[l_currentSwapchainIndex].Layout) {
				transitionImageLayoutCmd(l_cmdBuffer, (*m_frameBufferTextures)[l_currentSwapchainIndex].image.image,
					(*m_frameBufferTextures)[l_currentSwapchainIndex].format, (*m_frameBufferTextures)[l_currentSwapchainIndex].Layout,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
				(*m_frameBufferTextures)[l_currentSwapchainIndex].Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}
		}


		BeginRenderPass(m_renderPass.m_renderpass, m_framebufferHandles[l_currentSwapchainIndex], l_cmdBuffer, l_currentSwapchainIndex);
		vkCmdDraw(l_cmdBuffer, 6, 1, 0, 0);
		vkCmdEndRenderPass(l_cmdBuffer);

		if (nullptr != m_frameBufferTextures) {
			transitionImageLayoutCmd(l_cmdBuffer, (*m_frameBufferTextures)[l_currentSwapchainIndex].image.image,
				(*m_frameBufferTextures)[l_currentSwapchainIndex].format, (*m_frameBufferTextures)[l_currentSwapchainIndex].Layout,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			(*m_frameBufferTextures)[l_currentSwapchainIndex].Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
	}
}