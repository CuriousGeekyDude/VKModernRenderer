




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

		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
			m_swapchains[i] = &lv_vkResManager.RetrieveGpuTexture("Swapchain", i);
			m_bloomResults[i] = &lv_vkResManager.RetrieveGpuTexture("BlurSceneLinearInterpolated", i);
		}


		GeneratePipelineFromSpirvBinaries(l_spvPath);
		SetRenderPassAndFrameBuffer("PresentSwapchain");
		SetNodeToAppropriateRenderpass("PresentSwapchain", this);
		UpdateDescriptorSets();

		auto* lv_node = lv_frameGraph.RetrieveNode("PresentSwapchain");
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
			, { l_vtxShader, l_fragShader }, "GraphicsPipelinePresentSwapchain", lv_pipeInfo);
	}


	void PresentSwapchainRenderer::FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
		uint32_t l_currentSwapchainIndex)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();

		auto lv_framebuffer = lv_vkResManager.RetrieveGpuFramebuffer(m_framebufferHandles[l_currentSwapchainIndex]);

		BeginRenderPass(m_renderPass, lv_framebuffer, l_cmdBuffer, l_currentSwapchainIndex, 1);
		vkCmdDraw(l_cmdBuffer, 6, 1, 0, 0);
		vkCmdEndRenderPass(l_cmdBuffer);

		m_swapchains[l_currentSwapchainIndex]->Layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	}

	void PresentSwapchainRenderer::UpdateBuffers(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure)
	{

	}

	void PresentSwapchainRenderer::UpdateDescriptorSets()
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();

		std::vector<VkDescriptorImageInfo> lv_imageInfo{};
		lv_imageInfo.resize(lv_totalNumSwapchains);


		for (size_t i = 0; i < lv_imageInfo.size(); i++) {

			lv_imageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfo[i].imageView = m_bloomResults[i]->image.imageView0;
			lv_imageInfo[i].sampler = m_bloomResults[i]->sampler;

		}

		std::vector<VkWriteDescriptorSet> lv_writes;
		lv_writes.resize(lv_totalNumSwapchains);

		for (size_t i = 0; i < lv_writes.size(); ++i) {
			lv_writes[i].descriptorCount = 1;
			lv_writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i].dstArrayElement = 0;
			lv_writes[i].dstBinding = 0;
			lv_writes[i].dstSet = m_descriptorSets[i];
			lv_writes[i].pBufferInfo = nullptr;
			lv_writes[i].pImageInfo = &lv_imageInfo[i];
			lv_writes[i].pNext = nullptr;
			lv_writes[i].pTexelBufferView = nullptr;
			lv_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		}

		vkUpdateDescriptorSets(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device
			, lv_writes.size(), lv_writes.data(), 0, nullptr);

	}

}