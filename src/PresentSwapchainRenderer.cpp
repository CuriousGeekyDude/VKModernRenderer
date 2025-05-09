




#include "PresentSwapchainRenderer.hpp"



namespace RenderCore
{


	PresentSwapchainRenderer::PresentSwapchainRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator
		, const char* l_vtxShader
		, const char* l_fragShader
		, const char* l_spvPath)
		:Renderbase(l_vkContextCreator)
	{
		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();

		m_swapchains.resize(lv_totalNumSwapchains);
		m_bloomResults.resize(lv_totalNumSwapchains);
		m_imageInfo.resize(lv_totalNumSwapchains);
		m_writes.resize(lv_totalNumSwapchains);

		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
			m_swapchains[i] = &lv_vkResManager.RetrieveGpuTexture("Swapchain", i);
			m_bloomResults[i] = &lv_vkResManager.RetrieveGpuTexture("BlurSceneLinearInterpolated", i);
		}


		GeneratePipelineFromSpirvBinaries(l_spvPath);
		SetRenderPassAndFrameBuffer("FXAA");
		SetNodeToAppropriateRenderpass("FXAA", this);
		UpdateDescriptorSets();

		auto* lv_node = lv_frameGraph.RetrieveNode("FXAA");
		lv_frameGraph.IncrementNumNodesPerCmdBuffer(2);

		VulkanResourceManager::PipelineInfo lv_pipeInfo{};
		lv_pipeInfo.m_dynamicScissorState = false;
		lv_pipeInfo.m_enableWireframe = false;
		lv_pipeInfo.m_height = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferHeight;
		lv_pipeInfo.m_width = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferWidth;
		lv_pipeInfo.m_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		lv_pipeInfo.m_useBlending = false;
		lv_pipeInfo.m_useDepth = false;
		lv_pipeInfo.m_totalNumColorAttach = lv_node->m_outputResourcesHandles.size();


		m_graphicsPipeline = lv_vkResManager.CreateGraphicsPipeline(m_renderPass, m_pipelineLayout
			, { l_vtxShader, l_fragShader }, "GraphicsPipelineFXAA", lv_pipeInfo);

		m_debugTiledDeferredPresentSwapchain = lv_vkResManager.CreateGraphicsPipeline(m_renderPass, m_pipelineLayout
			, { l_vtxShader, "Shaders/DebugPresentTiledDeferredSwapchain.frag"}, "GraphicsPipelineDebugTiledPresentSwapchain", lv_pipeInfo);
	}




	void PresentSwapchainRenderer::SetSwitchToDebugTiled(bool l_switch)
	{
		m_switchToDebugTiledPipeline = l_switch;
	}


	void PresentSwapchainRenderer::FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
		uint32_t l_currentSwapchainIndex)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();

		auto lv_framebuffer = lv_vkResManager.RetrieveGpuFramebuffer(m_framebufferHandles[l_currentSwapchainIndex]);


		const VkRect2D rect{
			.offset = { 0, 0 },
			.extent = {.width = 704U,
			.height = 704U}
		};

		m_vulkanRenderContext.BeginRenderPass(l_cmdBuffer, m_renderPass, l_currentSwapchainIndex, rect,
			lv_framebuffer,
			0,
			nullptr);


		if (false == m_switchToDebugTiledPipeline) {
			vkCmdBindPipeline(l_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
		}
		else {
			vkCmdBindPipeline(l_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_debugTiledDeferredPresentSwapchain);
		}

		if (0 != m_descriptorSets.size()) {
			vkCmdBindDescriptorSets(l_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
				&m_descriptorSets[l_currentSwapchainIndex], 0, nullptr);
		}



		vkCmdDraw(l_cmdBuffer, 6, 1, 0, 0);
		vkCmdEndRenderPass(l_cmdBuffer);

		m_swapchains[l_currentSwapchainIndex]->Layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	}

	void PresentSwapchainRenderer::UpdateBuffers(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure)
	{

	}


	void PresentSwapchainRenderer::UpdateInputDescriptorImages(std::vector<VulkanTexture*>& l_newInputs)
	{
		
		for (size_t i = 0; i < m_imageInfo.size(); i++) {

			m_imageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			m_imageInfo[i].imageView = l_newInputs[i]->image.imageView0;
			m_imageInfo[i].sampler = l_newInputs[i]->sampler;

		}


		for (size_t i = 0; i < m_writes.size(); ++i) {
			m_writes[i].descriptorCount = 1;
			m_writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			m_writes[i].dstArrayElement = 0;
			m_writes[i].dstBinding = 0;
			m_writes[i].dstSet = m_descriptorSets[i];
			m_writes[i].pBufferInfo = nullptr;
			m_writes[i].pImageInfo = &m_imageInfo[i];
			m_writes[i].pNext = nullptr;
			m_writes[i].pTexelBufferView = nullptr;
			m_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		}

		vkUpdateDescriptorSets(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device
			, m_writes.size(), m_writes.data(), 0, nullptr);

	}
	

	void PresentSwapchainRenderer::UpdateDescriptorSets()
	{
		
		for (size_t i = 0; i < m_imageInfo.size(); i++) {

			m_imageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			m_imageInfo[i].imageView = m_bloomResults[i]->image.imageView0;
			m_imageInfo[i].sampler = m_bloomResults[i]->sampler;

		}

	

		for (size_t i = 0; i < m_writes.size(); ++i) {
			m_writes[i].descriptorCount = 1;
			m_writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			m_writes[i].dstArrayElement = 0;
			m_writes[i].dstBinding = 0;
			m_writes[i].dstSet = m_descriptorSets[i];
			m_writes[i].pBufferInfo = nullptr;
			m_writes[i].pImageInfo = &m_imageInfo[i];
			m_writes[i].pNext = nullptr;
			m_writes[i].pTexelBufferView = nullptr;
			m_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		}

		vkUpdateDescriptorSets(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device
			, m_writes.size(), m_writes.data(), 0, nullptr);

	}

}