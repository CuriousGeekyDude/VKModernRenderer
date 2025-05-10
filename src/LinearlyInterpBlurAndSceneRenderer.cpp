




#include "LinearlyInterpBlurAndSceneRenderer.hpp"



namespace RenderCore
{

	LinearlyInterpBlurAndSceneRenderer::LinearlyInterpBlurAndSceneRenderer
	(VulkanEngine::VulkanRenderContext& l_vkContextCreator
		, const char* l_vtxShader, const char* l_fragShader
		, const char* l_spv)
		:Renderbase(l_vkContextCreator)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();


		
		m_mipMapInputImages.resize(lv_totalNumSwapchains);
		m_descriptorImageViews.resize(2*lv_totalNumSwapchains);
		m_outputImages.resize(lv_totalNumSwapchains);

		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
			m_mipMapInputImages[i] = &lv_vkResManager.RetrieveGpuTexture("DeferredLightningColorTexture", i);
			m_outputImages[i] = &lv_vkResManager.RetrieveGpuTexture("BlurSceneLinearInterpolated", i);
		}


		for (size_t i = 0, j = 0; i < m_descriptorImageViews.size(); i += 2, ++j) {


			const VkImageViewCreateInfo lv_viewInfo =
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.image = m_mipMapInputImages[j]->image.image,
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = m_mipMapInputImages[j]->format,
				.subresourceRange =
				{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 1,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			};

			VULKAN_CHECK(vkCreateImageView(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device, &lv_viewInfo, nullptr, &m_descriptorImageViews[i]));



			const VkImageViewCreateInfo lv_viewInfo1 =
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.image = m_mipMapInputImages[j]->image.image,
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = m_mipMapInputImages[j]->format,
				.subresourceRange =
				{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			};

			VULKAN_CHECK(vkCreateImageView(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device, &lv_viewInfo1, nullptr, &m_descriptorImageViews[i + 1]));

		}

		SetRenderPassAndFrameBuffer("LinInterpBlurScene");
		GeneratePipelineFromSpirvBinaries(l_spv);
		SetNodeToAppropriateRenderpass("LinInterpBlurScene", this);
		UpdateDescriptorSets();



		auto* lv_node = lv_frameGraph.RetrieveNode("LinInterpBlurScene");
		lv_frameGraph.IncrementNumNodesPerCmdBuffer(2);



		VulkanResourceManager::PipelineInfo lv_pipeInfo{};
		lv_pipeInfo.m_dynamicScissorState = false;
		lv_pipeInfo.m_enableWireframe = false;
		lv_pipeInfo.m_height = 1024;
		lv_pipeInfo.m_width = 1024;
		lv_pipeInfo.m_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		lv_pipeInfo.m_useBlending = false;
		lv_pipeInfo.m_useDepth = false;
		lv_pipeInfo.m_totalNumColorAttach = lv_node->m_outputResourcesHandles.size();

		m_graphicsPipeline = lv_vkResManager.CreateGraphicsPipeline(m_renderPass, m_pipelineLayout
			, { l_vtxShader, l_fragShader }, "GraphicsPiplineLinInterpBlurScene", lv_pipeInfo);



	}

	void LinearlyInterpBlurAndSceneRenderer::FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
		uint32_t l_currentSwapchainIndex)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();

		auto lv_framebuffer = lv_vkResManager.RetrieveGpuFramebuffer(m_framebufferHandles[l_currentSwapchainIndex]);

		transitionImageLayoutCmd(l_cmdBuffer, m_outputImages[l_currentSwapchainIndex]->image.image
			, m_outputImages[l_currentSwapchainIndex]->format
			, m_outputImages[l_currentSwapchainIndex]->Layout
			, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		BeginRenderPass(m_renderPass, lv_framebuffer, l_cmdBuffer, l_currentSwapchainIndex
			, 1, 1024
			,1024);
		vkCmdDraw(l_cmdBuffer, 6, 1, 0, 0);
		vkCmdEndRenderPass(l_cmdBuffer);
		m_outputImages[l_currentSwapchainIndex]->Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	}


	void LinearlyInterpBlurAndSceneRenderer::UpdateBuffers(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure)
	{


	}

	void LinearlyInterpBlurAndSceneRenderer::UpdateDescriptorSets()
	{

		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();

		std::vector<VkDescriptorImageInfo> lv_imageInfo{};
		lv_imageInfo.resize(2*lv_totalNumSwapchains);

		for (size_t i = 0, j = 0; i < lv_imageInfo.size(); i+=2, ++j) {

			lv_imageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfo[i].imageView = m_descriptorImageViews[i];
			lv_imageInfo[i].sampler = m_mipMapInputImages[j]->sampler;

			lv_imageInfo[i + 1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfo[i + 1].imageView = m_descriptorImageViews[i + 1];
			lv_imageInfo[i + 1].sampler = m_mipMapInputImages[j]->sampler;

		}

		std::vector<VkWriteDescriptorSet> lv_writes{};
		lv_writes.resize(2 * lv_totalNumSwapchains);

		for (size_t i = 0, j = 0; i < lv_writes.size(); i += 2, ++j) {

			lv_writes[i].descriptorCount = 1;
			lv_writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i].dstArrayElement = 0;
			lv_writes[i].dstBinding = 0;
			lv_writes[i].dstSet = m_descriptorSets[j];
			lv_writes[i].pBufferInfo = nullptr;
			lv_writes[i].pImageInfo = &lv_imageInfo[i];
			lv_writes[i].pNext = nullptr;
			lv_writes[i].pTexelBufferView = nullptr;
			lv_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 1].descriptorCount = 1;
			lv_writes[i + 1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 1].dstArrayElement = 0;
			lv_writes[i + 1].dstBinding = 1;
			lv_writes[i + 1].dstSet = m_descriptorSets[j];
			lv_writes[i + 1].pBufferInfo = nullptr;
			lv_writes[i + 1].pImageInfo = &lv_imageInfo[i + 1];
			lv_writes[i + 1].pNext = nullptr;
			lv_writes[i + 1].pTexelBufferView = nullptr;
			lv_writes[i + 1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		}


		vkUpdateDescriptorSets(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device, lv_writes.size(), lv_writes.data(), 0, nullptr);

	}

}


