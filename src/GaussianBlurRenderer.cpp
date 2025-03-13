




#include "GaussianBlurRenderer.hpp"




namespace RenderCore
{

	GaussianBlurRenderer::GaussianBlurRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator
		, const char* l_vtxShader, const char* l_fragShader
		, const char* l_spv
		, const int l_blurTextureIndex
		, const bool l_referenceSceneOutputTexture
		,const char* l_nodeName)
		:Renderbase(l_vkContextCreator)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();
		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();

		assert(0 == l_blurTextureIndex || 1 == l_blurTextureIndex);

		m_blurOutputTextures.resize(lv_totalNumSwapchains);
		m_inputSampleTextures.resize(lv_totalNumSwapchains);

		if (0 == l_blurTextureIndex) {
			if (false == l_referenceSceneOutputTexture ) {
				for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
					m_blurOutputTextures[i] = &lv_vkResManager.RetrieveGpuTexture("GaussianBlurTexture0", i);
					m_inputSampleTextures[i] = &lv_vkResManager.RetrieveGpuTexture("GaussianBlurTexture1", i);
				}
			}
			else {
				for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
					m_blurOutputTextures[i] = &lv_vkResManager.RetrieveGpuTexture("GaussianBlurTexture0", i);
					m_inputSampleTextures[i] = &lv_vkResManager.RetrieveGpuTexture("ExtractBrightTexture", i);
				}
			}
		}
		else {
			if (false == l_referenceSceneOutputTexture) {
				for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
					m_blurOutputTextures[i] = &lv_vkResManager.RetrieveGpuTexture("GaussianBlurTexture1", i);
					m_inputSampleTextures[i] = &lv_vkResManager.RetrieveGpuTexture("GaussianBlurTexture0", i);
				}
			}
			else {
				for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
					m_blurOutputTextures[i] = &lv_vkResManager.RetrieveGpuTexture("GaussianBlurTexture1", i);
					m_inputSampleTextures[i] = &lv_vkResManager.RetrieveGpuTexture("ExtractBrightTexture", i);
				}
			}
		}


		GeneratePipelineFromSpirvBinaries(l_spv);
		SetRenderPassAndFrameBuffer(l_nodeName);
		SetNodeToAppropriateRenderpass(l_nodeName, this);
		UpdateDescriptorSets();

		auto* lv_node = lv_frameGraph.RetrieveNode(l_nodeName);
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

		std::string lv_stringNodeName{ "GraphicsPipeline" };
		lv_stringNodeName += l_nodeName;

		m_graphicsPipeline = lv_vkResManager.CreateGraphicsPipeline(m_renderPass, m_pipelineLayout
			, { l_vtxShader, l_fragShader }, lv_stringNodeName.c_str(), lv_pipeInfo);
		
	}

	void GaussianBlurRenderer::FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
		uint32_t l_currentSwapchainIndex)
	{

		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();

		transitionImageLayoutCmd(l_cmdBuffer, m_blurOutputTextures[l_currentSwapchainIndex]->image.image
			, m_blurOutputTextures[l_currentSwapchainIndex]->format, m_blurOutputTextures[l_currentSwapchainIndex]->Layout
			, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		transitionImageLayoutCmd(l_cmdBuffer, m_inputSampleTextures[l_currentSwapchainIndex]->image.image
			, m_inputSampleTextures[l_currentSwapchainIndex]->format, m_inputSampleTextures[l_currentSwapchainIndex]->Layout
			, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);



		m_blurOutputTextures[l_currentSwapchainIndex]->Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		m_inputSampleTextures[l_currentSwapchainIndex]->Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;


		auto lv_framebuffer = lv_vkResManager.RetrieveGpuFramebuffer(m_framebufferHandles[l_currentSwapchainIndex]);

		BeginRenderPass(m_renderPass, lv_framebuffer, l_cmdBuffer, l_currentSwapchainIndex, 1);
		vkCmdDraw(l_cmdBuffer, 6, 1, 0, 0);
		vkCmdEndRenderPass(l_cmdBuffer);


		m_blurOutputTextures[l_currentSwapchainIndex]->Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;



	}


	void GaussianBlurRenderer::UpdateBuffers(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure)
	{

	}


	void GaussianBlurRenderer::UpdateDescriptorSets()
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();

		std::vector<VkDescriptorImageInfo> lv_imageInfos;
		lv_imageInfos.resize(lv_totalNumSwapchains);


		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {

			lv_imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfos[i].imageView = m_inputSampleTextures[i]->image.imageView0;
			lv_imageInfos[i].sampler = m_inputSampleTextures[i]->sampler;

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
			lv_writes[i].pImageInfo = &lv_imageInfos[i];
			lv_writes[i].pNext = nullptr;
			lv_writes[i].pTexelBufferView = nullptr;
			lv_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		}



		vkUpdateDescriptorSets(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device, lv_writes.size(), lv_writes.data(), 0, nullptr);

	}



}