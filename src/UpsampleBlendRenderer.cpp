



#include "UpsampleBlendRenderer.hpp"


namespace RenderCore
{
	UpsampleBlendRenderer::UpsampleBlendRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator
		, const char* l_vtxShader
		, const char* l_fragShader
		, const char* l_spvPath
		, const char* l_rendererName
		, uint32_t l_mipLevelTtoRenderTo)
		:Renderbase(l_vkContextCreator)
		, m_mipLevelToRenderTo(l_mipLevelTtoRenderTo)
	{

		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();


		m_newFramebuffers.resize(lv_totalNumSwapchains);
		m_framebufferImageViews.resize(lv_totalNumSwapchains);
		m_mipchainDimensions.resize(m_totalNumMipLevels);
		m_mipMapInputOutputImages.resize(lv_totalNumSwapchains);
		m_descriptorImageViews.resize(lv_totalNumSwapchains);


		std::string lv_rendererName{ l_rendererName };

		std::string lv_uniformBufferName{ "UniformBuffer" + lv_rendererName };

		m_uniformBufferGpu = &lv_vkResManager.CreateBuffer(sizeof(UniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
			, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			, lv_uniformBufferName.c_str());




		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
			m_mipMapInputOutputImages[i] = &lv_vkResManager.RetrieveGpuTexture("DeferredLightningColorTexture", i);
		}




		m_mipchainDimensions[0].x = (uint32_t)m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferWidth;
		m_mipchainDimensions[0].y = (uint32_t)m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferHeight;
		for (uint32_t i = 1; i < m_totalNumMipLevels; ++i) {
			m_mipchainDimensions[i].x = m_mipchainDimensions[i - 1].x / 2;
			m_mipchainDimensions[i].y = m_mipchainDimensions[i - 1].y / 2;
		}



		UniformBuffer lv_uniform{};
		lv_uniform.m_indexMipchain = m_mipLevelToRenderTo;
		lv_uniform.m_radius = 0.005f;
		lv_uniform.m_mipchainDimensions = glm::vec4{ m_mipchainDimensions[m_mipLevelToRenderTo + 1].x,m_mipchainDimensions[m_mipLevelToRenderTo + 1].y, 1.f, 1.f };

		memcpy(m_uniformBufferGpu->ptr, &lv_uniform, sizeof(UniformBuffer));


		SetRenderPassAndFrameBuffer(l_rendererName);



		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {


			VkImageView lv_mipChainView{};
			const VkImageViewCreateInfo lv_viewInfo =
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.image = m_mipMapInputOutputImages[i]->image.image,
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = m_mipMapInputOutputImages[i]->format,
				.subresourceRange =
				{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = l_mipLevelTtoRenderTo,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			};

			VULKAN_CHECK(vkCreateImageView(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device, &lv_viewInfo, nullptr, &lv_mipChainView));

			m_framebufferImageViews[i] = lv_mipChainView;


			VkImageView lv_descriptorMipChainView{};
			const VkImageViewCreateInfo lv_viewInfo1 =
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.image = m_mipMapInputOutputImages[i]->image.image,
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = m_mipMapInputOutputImages[i]->format,
				.subresourceRange =
				{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = l_mipLevelTtoRenderTo + 1,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			};

			VULKAN_CHECK(vkCreateImageView(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device, &lv_viewInfo1, nullptr, &lv_descriptorMipChainView));

			m_descriptorImageViews[i] = lv_descriptorMipChainView;


			VkFramebufferCreateInfo lv_frameBufferCreateInfo{};
			lv_frameBufferCreateInfo.attachmentCount = 1;
			lv_frameBufferCreateInfo.height = (m_mipchainDimensions[l_mipLevelTtoRenderTo].y);
			lv_frameBufferCreateInfo.width = (m_mipchainDimensions[l_mipLevelTtoRenderTo].x);
			lv_frameBufferCreateInfo.pAttachments = &lv_mipChainView;
			lv_frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			lv_frameBufferCreateInfo.renderPass = m_renderPass;
			lv_frameBufferCreateInfo.layers = 1U;

			VULKAN_CHECK(vkCreateFramebuffer(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device, &lv_frameBufferCreateInfo, nullptr, &m_newFramebuffers[i]));





		}


		GeneratePipelineFromSpirvBinaries(l_spvPath);
		SetNodeToAppropriateRenderpass(l_rendererName, this);
		UpdateDescriptorSets();



		auto* lv_node = lv_frameGraph.RetrieveNode(l_rendererName);
		lv_frameGraph.IncrementNumNodesPerCmdBuffer(2);


		VulkanResourceManager::PipelineInfo lv_pipeInfo{};
		lv_pipeInfo.m_dynamicScissorState = false;
		lv_pipeInfo.m_enableWireframe = false;
		lv_pipeInfo.m_height = m_mipchainDimensions[l_mipLevelTtoRenderTo].x;
		lv_pipeInfo.m_width = m_mipchainDimensions[l_mipLevelTtoRenderTo].y;
		lv_pipeInfo.m_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		lv_pipeInfo.m_useBlending = true;
		lv_pipeInfo.m_useDepth = false;
		lv_pipeInfo.m_totalNumColorAttach = lv_node->m_outputResourcesHandles.size();

		std::string lv_graphicsPipelineName{ "GraphicsPipeline" + lv_rendererName };

		m_graphicsPipeline = lv_vkResManager.CreateGraphicsPipeline(m_renderPass, m_pipelineLayout
			, { l_vtxShader, l_fragShader }, lv_graphicsPipelineName.c_str(), lv_pipeInfo);




	}



	void UpsampleBlendRenderer::FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
		uint32_t l_currentSwapchainIndex)
	{

		auto lv_framebuffer = m_newFramebuffers[l_currentSwapchainIndex];

		transitionImageLayoutCmd(l_cmdBuffer, m_mipMapInputOutputImages[l_currentSwapchainIndex]->image.image
			, m_mipMapInputOutputImages[l_currentSwapchainIndex]->format
			, m_mipMapInputOutputImages[l_currentSwapchainIndex]->Layout
			, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1, 1, 0, m_mipLevelToRenderTo);


		BeginRenderPass(m_renderPass, lv_framebuffer, l_cmdBuffer, l_currentSwapchainIndex, 1, m_mipchainDimensions[m_mipLevelToRenderTo].x, m_mipchainDimensions[m_mipLevelToRenderTo].y);
		vkCmdDraw(l_cmdBuffer, 6, 1, 0, 0);
		vkCmdEndRenderPass(l_cmdBuffer);

	}


	void UpsampleBlendRenderer::UpdateBuffers(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure)
	{
		

	}

	void UpsampleBlendRenderer::UpdateDescriptorSets()
	{

		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();

		VkDescriptorBufferInfo lv_bufferInfo{};
		lv_bufferInfo.buffer = m_uniformBufferGpu->buffer;
		lv_bufferInfo.offset = 0;
		lv_bufferInfo.range = VK_WHOLE_SIZE;


		std::vector<VkDescriptorImageInfo> lv_imageInfo{};
		lv_imageInfo.resize(lv_totalNumSwapchains);

		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {

			lv_imageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfo[i].imageView = m_descriptorImageViews[i];
			lv_imageInfo[i].sampler = m_mipMapInputOutputImages[i]->sampler;

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
			lv_writes[i].pImageInfo = &lv_imageInfo[j];
			lv_writes[i].pNext = nullptr;
			lv_writes[i].pTexelBufferView = nullptr;
			lv_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 1].descriptorCount = 1;
			lv_writes[i + 1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			lv_writes[i + 1].dstArrayElement = 0;
			lv_writes[i + 1].dstBinding = 1;
			lv_writes[i + 1].dstSet = m_descriptorSets[j];
			lv_writes[i + 1].pBufferInfo = &lv_bufferInfo;
			lv_writes[i + 1].pImageInfo = nullptr;
			lv_writes[i + 1].pNext = nullptr;
			lv_writes[i + 1].pTexelBufferView = nullptr;
			lv_writes[i + 1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		}


		vkUpdateDescriptorSets(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device, lv_writes.size(), lv_writes.data(), 0, nullptr);

	}


	UpsampleBlendRenderer::~UpsampleBlendRenderer()
	{

		for (auto l_framebuffer : m_newFramebuffers) {
			vkDestroyFramebuffer(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device, l_framebuffer, nullptr);
		}

		for (auto l_imageView : m_framebufferImageViews) {
			vkDestroyImageView(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device, l_imageView, nullptr);
		}

		for (auto l_imageView : m_descriptorImageViews) {
			vkDestroyImageView(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device, l_imageView, nullptr);
		}

	}

}