




#include "BoxBlurRenderer.hpp"


namespace RenderCore
{

	BoxBlurRenderer::BoxBlurRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator
		, const char* l_vtxShader, const char* l_fragmentShader
		, const char* l_spvFile)
		:Renderbase(l_vkContextCreator)
	{

		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();
		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();

		m_ssaoTextures.resize(lv_totalNumSwapchains);
		m_frameBufferTextures.resize(lv_totalNumSwapchains);

		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
			m_ssaoTextures[i] = &lv_vkResManager.RetrieveGpuTexture("OcclusionFactor", i);
			m_frameBufferTextures[i] = &lv_vkResManager.RetrieveGpuTexture("BoxBlurTexture", i);
		}


		GeneratePipelineFromSpirvBinaries(l_spvFile);
		SetRenderPassAndFrameBuffer("BoxBlur");
		SetNodeToAppropriateRenderpass("BoxBlur", this);
		UpdateDescriptorSets();
		auto* lv_node = lv_frameGraph.RetrieveNode("BoxBlur");
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
			, { l_vtxShader, l_fragmentShader }, "GraphicsPipelineBoxBlurRenderer", lv_pipeInfo);

	}


	void BoxBlurRenderer::UpdateDescriptorSets()
	{
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();

		std::vector<VkDescriptorImageInfo> lv_imageDescriptors{};
		lv_imageDescriptors.resize(lv_totalNumSwapchains);

		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
			lv_imageDescriptors[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageDescriptors[i].imageView = m_ssaoTextures[i]->image.imageView0;
			lv_imageDescriptors[i].sampler = m_ssaoTextures[i]->sampler;
		}


		std::vector<VkWriteDescriptorSet> lv_writes{};
		lv_writes.resize(lv_totalNumSwapchains);

		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
			lv_writes[i].descriptorCount = 1;
			lv_writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i].dstArrayElement = 0;
			lv_writes[i].dstBinding = 0;
			lv_writes[i].dstSet = m_descriptorSets[i];
			lv_writes[i].pBufferInfo = nullptr;
			lv_writes[i].pImageInfo = &lv_imageDescriptors[i];
			lv_writes[i].pNext = nullptr;
			lv_writes[i].pTexelBufferView = nullptr;
			lv_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		}

		vkUpdateDescriptorSets(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device, lv_writes.size() , lv_writes.data(), 0, nullptr);
	}

	void BoxBlurRenderer::UpdateBuffers(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure)
	{

	}


	void BoxBlurRenderer::FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
		uint32_t l_currentSwapchainIndex)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_framebuffer = lv_vkResManager.RetrieveGpuFramebuffer(m_framebufferHandles[l_currentSwapchainIndex]);

		transitionImageLayoutCmd(l_cmdBuffer, m_ssaoTextures[l_currentSwapchainIndex]->image.image
			, m_ssaoTextures[l_currentSwapchainIndex]->format
			, m_ssaoTextures[l_currentSwapchainIndex]->Layout
			, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		transitionImageLayoutCmd(l_cmdBuffer, m_frameBufferTextures[l_currentSwapchainIndex]->image.image
			, m_frameBufferTextures[l_currentSwapchainIndex]->format
			, m_frameBufferTextures[l_currentSwapchainIndex]->Layout
			, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		m_ssaoTextures[l_currentSwapchainIndex]->Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		m_frameBufferTextures[l_currentSwapchainIndex]->Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		BeginRenderPass(m_renderPass, lv_framebuffer, l_cmdBuffer, l_currentSwapchainIndex, 1);
		vkCmdDraw(l_cmdBuffer, 6, 1, 0, 0);
		vkCmdEndRenderPass(l_cmdBuffer);

		m_frameBufferTextures[l_currentSwapchainIndex]->Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		transitionImageLayoutCmd(l_cmdBuffer, m_frameBufferTextures[l_currentSwapchainIndex]->image.image
			, m_frameBufferTextures[l_currentSwapchainIndex]->format
			, m_frameBufferTextures[l_currentSwapchainIndex]->Layout
			, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	}

}