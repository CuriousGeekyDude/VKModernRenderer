



#include "BloomBlendBlurAndSceneRenderer.hpp"


namespace RenderCore
{

	BloomBlendBlurAndSceneRenderer::BloomBlendBlurAndSceneRenderer
	(VulkanEngine::VulkanRenderContext& l_vkContextCreator
		, const char* l_vtxShader, const char* l_fragShader
		, const char* l_spv
		, const int l_blurTextureIndex)
		:Renderbase(l_vkContextCreator)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();
		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();

		m_swapchainTexture.resize(lv_totalNumSwapchains);
		m_gaussianBlurredTextures.resize(lv_totalNumSwapchains);
		m_deferredLightnintOutputTextures.resize(lv_totalNumSwapchains);

		assert(0 == l_blurTextureIndex || 1 == l_blurTextureIndex);

		if (1 == l_blurTextureIndex) {

			for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
				m_swapchainTexture[i] = &lv_vkResManager.RetrieveGpuTexture("Swapchain", i);
				m_gaussianBlurredTextures[i] = &lv_vkResManager.RetrieveGpuTexture("GaussianBlurTexture1", i);
				m_deferredLightnintOutputTextures[i] = &lv_vkResManager.RetrieveGpuTexture("DeferredLightningColorTexture", i);
			}

		}
		else {
			for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
				m_swapchainTexture[i] = &lv_vkResManager.RetrieveGpuTexture("Swapchain", i);
				m_gaussianBlurredTextures[i] = &lv_vkResManager.RetrieveGpuTexture("GaussianBlurTexture0", i);
				m_deferredLightnintOutputTextures[i] = &lv_vkResManager.RetrieveGpuTexture("DeferredLightningColorTexture", i);
			}
		}


		GeneratePipelineFromSpirvBinaries(l_spv);
		SetRenderPassAndFrameBuffer("BloomBlendBlurAndScene");
		SetNodeToAppropriateRenderpass("BloomBlendBlurAndScene", this);
		UpdateDescriptorSets();

		auto* lv_node = lv_frameGraph.RetrieveNode("BloomBlendBlurAndScene");
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
			, { l_vtxShader, l_fragShader }, "GraphicsPipelineBloomBlendBlurAndScene", lv_pipeInfo);
	}

	void BloomBlendBlurAndSceneRenderer::FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
		uint32_t l_currentSwapchainIndex)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		


		auto lv_framebuffer = lv_vkResManager.RetrieveGpuFramebuffer(m_framebufferHandles[l_currentSwapchainIndex]);

		BeginRenderPass(m_renderPass, lv_framebuffer, l_cmdBuffer, l_currentSwapchainIndex, 1);
		vkCmdDraw(l_cmdBuffer, 6, 1, 0, 0);
		vkCmdEndRenderPass(l_cmdBuffer);


		m_swapchainTexture[l_currentSwapchainIndex]->Layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}


	void BloomBlendBlurAndSceneRenderer::UpdateBuffers(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure) 
	{

	}


	void BloomBlendBlurAndSceneRenderer::UpdateDescriptorSets()
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();

		std::vector<VkDescriptorImageInfo> lv_imageInfos{};
		lv_imageInfos.resize(lv_totalNumSwapchains * 2);

		for (size_t i = 0, j = 0; i < lv_imageInfos.size(); i+=2, ++j) {

			lv_imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfos[i].imageView = m_deferredLightnintOutputTextures[j]->image.imageView0;
			lv_imageInfos[i].sampler = m_deferredLightnintOutputTextures[j]->sampler;

			lv_imageInfos[i+1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfos[i+1].imageView = m_gaussianBlurredTextures[j]->image.imageView0;
			lv_imageInfos[i+1].sampler = m_gaussianBlurredTextures[j]->sampler;
		}

		std::vector<VkWriteDescriptorSet> lv_writes{};
		lv_writes.resize(lv_totalNumSwapchains*2);

		for (size_t i = 0, j = 0; i < lv_writes.size(); i += 2, ++j) {

			lv_writes[i].descriptorCount = 1;
			lv_writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i].dstArrayElement = 0;
			lv_writes[i].dstBinding = 0;
			lv_writes[i].dstSet = m_descriptorSets[j];
			lv_writes[i].pBufferInfo = nullptr;
			lv_writes[i].pImageInfo = &lv_imageInfos[i];
			lv_writes[i].pNext = nullptr;
			lv_writes[i].pTexelBufferView = nullptr;
			lv_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i+1].descriptorCount = 1;
			lv_writes[i+1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i+1].dstArrayElement = 0;
			lv_writes[i+1].dstBinding = 1;
			lv_writes[i+1].dstSet = m_descriptorSets[j];
			lv_writes[i+1].pBufferInfo = nullptr;
			lv_writes[i+1].pImageInfo = &lv_imageInfos[i+1];
			lv_writes[i+1].pNext = nullptr;
			lv_writes[i+1].pTexelBufferView = nullptr;
			lv_writes[i+1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		}

		vkUpdateDescriptorSets(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device, lv_writes.size(), lv_writes.data(), 0, nullptr);

	}

}