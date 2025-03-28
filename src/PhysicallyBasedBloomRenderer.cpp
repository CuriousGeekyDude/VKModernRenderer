




#include "PhysicallyBasedBloomRenderer.hpp"
#include <format>
#include "CameraStructure.hpp"


namespace RenderCore
{
	PhysicallyBasedBloomRenderer::PhysicallyBasedBloomRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator
		, const char* l_vtxShader
		, const char* l_fragShader
		, const char* l_spvPath)
		:Renderbase(l_vkContextCreator)
	{

		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();


		std::vector<VkFramebuffer> lv_newFrameBufferHandles{};
		lv_newFrameBufferHandles.resize(m_totalNumMipLevels *lv_totalNumSwapchains);
		m_framebufferImageViews.reserve(m_totalNumMipLevels * lv_totalNumSwapchains);
		m_mipchainDimensions.resize(m_totalNumMipLevels);
		m_mipMapInputOutputImages.resize(lv_totalNumSwapchains);
		m_currentMipchainIndexToSample.resize(lv_totalNumSwapchains);
		memset(m_currentMipchainIndexToSample.data(), 0, sizeof(uint32_t) * lv_totalNumSwapchains);

		m_uniformBufferGpu = &lv_vkResManager.CreateBuffer(sizeof(UniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
														, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
														, "UniformBufferPhysicallyBloomRenderer");



		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
			m_mipMapInputOutputImages[i] = &lv_vkResManager.RetrieveGpuTexture("DeferredLightningColorTexture", i);
			lv_newFrameBufferHandles[m_totalNumMipLevels * i] = lv_vkResManager.RetrieveGpuFramebuffer("PhysicallybasedBloomFramebuffer", i);
		}

		m_mipchainDimensions[0].x = (uint32_t)m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferWidth;
		m_mipchainDimensions[0].y = (uint32_t)m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferHeight;
		for (uint32_t i = 1; i < m_totalNumMipLevels; ++i) {
			m_mipchainDimensions[i].x = m_mipchainDimensions[i - 1].x / 2;
			m_mipchainDimensions[i].y = m_mipchainDimensions[i - 1].y / 2;
		}



		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {



			for (size_t j = 0; j < m_totalNumMipLevels; ++j) {

				if (j == 0) { continue; }
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
						.baseMipLevel = j,
						.levelCount = 1,
						.baseArrayLayer = 0,
						.layerCount = 1
					}
				};

				VULKAN_CHECK(vkCreateImageView(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device, &lv_viewInfo, nullptr, &lv_mipChainView));

				m_framebufferImageViews.push_back(lv_mipChainView);

				

				VkFramebufferCreateInfo lv_frameBufferCreateInfo{};
				lv_frameBufferCreateInfo.attachmentCount = 1;
				lv_frameBufferCreateInfo.height = (m_mipchainDimensions[j].y);
				lv_frameBufferCreateInfo.width = (m_mipchainDimensions[j].x);
				lv_frameBufferCreateInfo.pAttachments = &lv_mipChainView;
				lv_frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				lv_frameBufferCreateInfo.renderPass = m_renderPass;
				lv_frameBufferCreateInfo.layers = 1U;

				VULKAN_CHECK(vkCreateFramebuffer(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device, &lv_frameBufferCreateInfo, nullptr, &m_newFramebuffers[i* m_totalNumMipLevels + j]));
				

			}
			

		}


		GeneratePipelineFromSpirvBinaries(l_spvPath);
		SetRenderPassAndFrameBuffer("PhysicallybasedBloom");
		SetNodeToAppropriateRenderpass("PhysicallybasedBloom", this);
		UpdateDescriptorSets();

		

		auto* lv_node = lv_frameGraph.RetrieveNode("PhysicallybasedBloom");
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
			, { l_vtxShader, l_fragShader }, "GraphicsPipelineDeferredLightning", lv_pipeInfo);




	}



	void PhysicallyBasedBloomRenderer::FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
		uint32_t l_currentSwapchainIndex)
	{


		for (uint32_t i = 1; i < m_totalNumMipLevels; ++i) {
			
			auto lv_framebuffer = m_newFramebuffers[l_currentSwapchainIndex * m_totalNumMipLevels + i];

			transitionImageLayoutCmd(l_cmdBuffer, m_mipMapInputOutputImages[l_currentSwapchainIndex]->image.image
				, m_mipMapInputOutputImages[l_currentSwapchainIndex]->format
				, m_mipMapInputOutputImages[l_currentSwapchainIndex]->Layout
				, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,1, 1, 1, i);

			BeginRenderPass(m_renderPass, lv_framebuffer, l_cmdBuffer, l_currentSwapchainIndex, 1);
			vkCmdDraw(l_cmdBuffer, 6, 1, 0, 0);
			vkCmdEndRenderPass(l_cmdBuffer);

			transitionImageLayoutCmd(l_cmdBuffer, m_mipMapInputOutputImages[l_currentSwapchainIndex]->image.image
				, m_mipMapInputOutputImages[l_currentSwapchainIndex]->format
				, m_mipMapInputOutputImages[l_currentSwapchainIndex]->Layout
				, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 1, 1, i);

			UpdateBuffers(l_currentSwapchainIndex, {});
		}



	}

	void PhysicallyBasedBloomRenderer::UpdateBuffers(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure)
	{
		auto lv_currentIndex = m_currentMipchainIndexToSample[l_currentSwapchainIndex];
		UniformBuffer lv_uniform{};
		lv_uniform.m_indexMipchain = lv_currentIndex;
		lv_uniform.m_mipchainDimensions = glm::vec4{ m_mipchainDimensions[lv_currentIndex].x,m_mipchainDimensions[lv_currentIndex].y, 1.f, 1.f };

		memcpy(m_uniformBufferGpu->ptr, &lv_uniform, sizeof(UniformBuffer));

		++m_currentMipchainIndexToSample[l_currentSwapchainIndex];
		m_currentMipchainIndexToSample[l_currentSwapchainIndex] = m_currentMipchainIndexToSample[l_currentSwapchainIndex] % m_totalNumMipLevels;


	}

	void PhysicallyBasedBloomRenderer::UpdateDescriptorSets()
	{

		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();

		VkDescriptorBufferInfo lv_bufferInfo{};
		lv_bufferInfo.buffer = m_uniformBufferGpu->buffer;
		lv_bufferInfo.offset = 0;
		lv_bufferInfo.range = VK_WHOLE_SIZE;


		std::vector<VkDescriptorImageInfo> lv_imageInfo{};
		lv_imageInfo.resize(lv_totalNumSwapchains*m_totalNumMipLevels);

		for (size_t i = 0, j = 0; i < lv_imageInfo.size(); i+=m_totalNumMipLevels, ++j) {

			lv_imageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfo[i].imageView = m_framebufferImageViews[i];
			lv_imageInfo[i].sampler = m_mipMapInputOutputImages[j]->sampler;

			lv_imageInfo[i + 1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfo[i + 1].imageView = m_framebufferImageViews[i + 1];
			lv_imageInfo[i + 1].sampler = m_mipMapInputOutputImages[j]->sampler;

			lv_imageInfo[i + 2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfo[i + 2].imageView = m_framebufferImageViews[i + 2];
			lv_imageInfo[i + 2].sampler = m_mipMapInputOutputImages[j]->sampler;

			lv_imageInfo[i + 3].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfo[i + 3].imageView = m_framebufferImageViews[i + 3];
			lv_imageInfo[i + 3].sampler = m_mipMapInputOutputImages[j]->sampler;

			lv_imageInfo[i + 4].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfo[i + 4].imageView = m_framebufferImageViews[i + 4];
			lv_imageInfo[i + 4].sampler = m_mipMapInputOutputImages[j]->sampler;

			lv_imageInfo[i + 5].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfo[i + 5].imageView = m_framebufferImageViews[i + 5];
			lv_imageInfo[i + 5].sampler = m_mipMapInputOutputImages[j]->sampler;

		}

		std::vector<VkWriteDescriptorSet> lv_writes{};
		lv_writes.resize((m_totalNumMipLevels + 1) * lv_totalNumSwapchains);

		for (size_t i = 0, j = 0; i < lv_writes.size(); i += m_totalNumMipLevels + 1, ++j) {

			lv_writes[i].descriptorCount = 1;
			lv_writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i].dstArrayElement = 0;
			lv_writes[i].dstBinding = 0;
			lv_writes[i].dstSet = m_descriptorSets[j];
			lv_writes[i].pBufferInfo = nullptr;
			lv_writes[i].pImageInfo = &lv_imageInfo[m_totalNumMipLevels * j];
			lv_writes[i].pNext = nullptr;
			lv_writes[i].pTexelBufferView = nullptr;
			lv_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 1].descriptorCount = 1;
			lv_writes[i + 1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 1].dstArrayElement = 0;
			lv_writes[i + 1].dstBinding = 1;
			lv_writes[i + 1].dstSet = m_descriptorSets[j];
			lv_writes[i + 1].pBufferInfo = nullptr;
			lv_writes[i + 1].pImageInfo = &lv_imageInfo[m_totalNumMipLevels * j + 1];
			lv_writes[i + 1].pNext = nullptr;
			lv_writes[i + 1].pTexelBufferView = nullptr;
			lv_writes[i + 1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 2].descriptorCount = 1;
			lv_writes[i + 2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 2].dstArrayElement = 0;
			lv_writes[i + 2].dstBinding = 2;
			lv_writes[i + 2].dstSet = m_descriptorSets[j];
			lv_writes[i + 2].pBufferInfo = nullptr;
			lv_writes[i + 2].pImageInfo = &lv_imageInfo[m_totalNumMipLevels * j + 2];
			lv_writes[i + 2].pNext = nullptr;
			lv_writes[i + 2].pTexelBufferView = nullptr;
			lv_writes[i + 2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 3].descriptorCount = 1;
			lv_writes[i + 3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 3].dstArrayElement = 0;
			lv_writes[i + 3].dstBinding = 3;
			lv_writes[i + 3].dstSet = m_descriptorSets[j];
			lv_writes[i + 3].pBufferInfo = nullptr;
			lv_writes[i + 3].pImageInfo = &lv_imageInfo[m_totalNumMipLevels * j + 3];
			lv_writes[i + 3].pNext = nullptr;
			lv_writes[i + 3].pTexelBufferView = nullptr;
			lv_writes[i + 3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 4].descriptorCount = 1;
			lv_writes[i + 4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 4].dstArrayElement = 0;
			lv_writes[i + 4].dstBinding = 4;
			lv_writes[i + 4].dstSet = m_descriptorSets[j];
			lv_writes[i + 4].pBufferInfo = nullptr;
			lv_writes[i + 4].pImageInfo = &lv_imageInfo[m_totalNumMipLevels * j + 4];
			lv_writes[i + 4].pNext = nullptr;
			lv_writes[i + 4].pTexelBufferView = nullptr;
			lv_writes[i + 4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 5].descriptorCount = 1;
			lv_writes[i + 5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 5].dstArrayElement = 0;
			lv_writes[i + 5].dstBinding = 5;
			lv_writes[i + 5].dstSet = m_descriptorSets[j];
			lv_writes[i + 5].pBufferInfo = nullptr;
			lv_writes[i + 5].pImageInfo = &lv_imageInfo[m_totalNumMipLevels * j + 5];
			lv_writes[i + 5].pNext = nullptr;
			lv_writes[i + 5].pTexelBufferView = nullptr;
			lv_writes[i + 5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 6].descriptorCount = 1;
			lv_writes[i + 6].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			lv_writes[i + 6].dstArrayElement = 0;
			lv_writes[i + 6].dstBinding = 6;
			lv_writes[i + 6].dstSet = m_descriptorSets[j];
			lv_writes[i + 6].pBufferInfo = &lv_bufferInfo;
			lv_writes[i + 6].pImageInfo = nullptr;
			lv_writes[i + 6].pNext = nullptr;
			lv_writes[i + 6].pTexelBufferView = nullptr;
			lv_writes[i + 6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		}


		vkUpdateDescriptorSets(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device, lv_writes.size(), lv_writes.data(), 0, nullptr);

	}
}