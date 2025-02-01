

#include "CompositeRenderer.hpp"




namespace RenderCore
{

	CompositeRenderer::CompositeRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator) 
		: Renderbase(l_vkContextCreator)
	{
		
	}


	void CompositeRenderer::FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
		uint32_t l_currentSwapchainIndex)
	{

		for (auto& l_renderer : m_renderers) {

			if (true == l_renderer.m_enabled) {
				l_renderer.m_rendererBase.FillCommandBuffer(l_cmdBuffer, l_currentSwapchainIndex);
			}
		}
	}

	void CompositeRenderer::UpdateUniformBuffers(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure)
	{
		for (auto& l_renderer : m_renderers) {
			if (true == l_renderer.m_enabled) {
				l_renderer.m_rendererBase.UpdateUniformBuffers(l_currentSwapchainIndex, l_cameraStructure);
			}
		}
	}


	void CompositeRenderer::UpdateDescriptorSets()
	{

	}
}