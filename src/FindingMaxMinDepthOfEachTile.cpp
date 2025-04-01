


#include "FindingMaxMinDepthOfEachTile.hpp"


namespace RenderCore
{


	FindingMaxMinDepthOfEachTile::FindingMaxMinDepthOfEachTile(VulkanEngine::VulkanRenderContext& l_vkContextCreator
		, const char* l_computeShader
		, const char* l_spv)
		:Renderbase(l_vkContextCreator)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();


		m_depthTextures.resize(lv_totalNumSwapchains);


		for (size_t i = 0; i < m_depthTextures.size(); ++i) {
			m_depthTextures[i] = &lv_vkResManager.RetrieveGpuTexture("Depth", i);
		}


		auto lv_debugDepthHandle = lv_vkResManager.CreateBufferWithHandle(sizeof(float) * 2 * 44 * 44
			, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			, "DebugDepthBufferTiledDeferred");

		m_debugDepthBuffer = &lv_vkResManager.RetrieveGpuBuffer(lv_debugDepthHandle);

		std::vector<float> lv_temp;
		lv_temp.resize(2 * 44 * 44);

		lv_vkResManager.CopyDataToLocalBuffer(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_mainQueue2,
			m_vulkanRenderContext.GetContextCreator().m_vkDev.m_mainCommandBuffer1[0], lv_debugDepthHandle,
			lv_temp.data());

		GeneratePipelineFromSpirvBinaries(l_spv);
		SetNodeToAppropriateRenderpass("FindingMaxMinDepthOfEachTile", this);
		UpdateDescriptorSets();
		lv_frameGraph.IncrementNumNodesPerCmdBuffer(2);

		m_computePipeline = lv_vkResManager.CreateComputePipeline
		(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device
			, l_computeShader, m_pipelineLayout);

	}

	void FindingMaxMinDepthOfEachTile::FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
		uint32_t l_currentSwapchainIndex)
	{


		VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.oldLayout = m_depthTextures[l_currentSwapchainIndex]->Layout,
		.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = m_depthTextures[l_currentSwapchainIndex]->image.image,
		.subresourceRange = VkImageSubresourceRange {
			.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
		};



		VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;


		transitionImageLayoutCmd(l_cmdBuffer, m_depthTextures[l_currentSwapchainIndex]->image.image
			, m_depthTextures[l_currentSwapchainIndex]->format, m_depthTextures[l_currentSwapchainIndex]->Layout
		, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


		vkCmdPipelineBarrier(
			l_cmdBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);


		vkCmdBindPipeline(l_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);
		vkCmdBindDescriptorSets(l_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1,
			&m_descriptorSets[l_currentSwapchainIndex], 0, nullptr);

		uint32_t lv_width = (uint32_t)m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferWidth;
		uint32_t lv_height = (uint32_t)m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferHeight;


		vkCmdDispatch(l_cmdBuffer, (uint32_t)lv_width / (uint32_t)16, (uint32_t)lv_height / (uint32_t)16, 1);

		barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_SHADER_READ_BIT ,
		.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT ,
		.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = m_depthTextures[l_currentSwapchainIndex]->image.image,
		.subresourceRange = VkImageSubresourceRange {
			.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
		};



		sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

		vkCmdPipelineBarrier(
			l_cmdBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		m_depthTextures[l_currentSwapchainIndex]->Layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}


	void FindingMaxMinDepthOfEachTile::UpdateBuffers(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure)
	{

	}


	void FindingMaxMinDepthOfEachTile::UpdateDescriptorSets()
	{

		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();

		VkDescriptorBufferInfo lv_bufferInfo;
		lv_bufferInfo.buffer = m_debugDepthBuffer->buffer;
		lv_bufferInfo.offset = 0;
		lv_bufferInfo.range = VK_WHOLE_SIZE;

		std::vector<VkDescriptorImageInfo> lv_imageInfo;
		lv_imageInfo.resize(lv_totalNumSwapchains);

		for (size_t i = 0; i < lv_imageInfo.size(); ++i) {

			lv_imageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfo[i].imageView = m_depthTextures[i]->image.imageView0;
			lv_imageInfo[i].sampler = m_depthTextures[i]->sampler;

		}

		std::vector<VkWriteDescriptorSet> lv_writes;
		lv_writes.resize(lv_totalNumSwapchains*2);

		for (size_t i = 0, j = 0; i < lv_writes.size(); i+=2, ++j) {


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

			lv_writes[i+1].descriptorCount = 1;
			lv_writes[i+1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			lv_writes[i+1].dstArrayElement = 0;
			lv_writes[i+1].dstBinding = 1;
			lv_writes[i+1].dstSet = m_descriptorSets[j];
			lv_writes[i+1].pBufferInfo = &lv_bufferInfo;
			lv_writes[i+1].pImageInfo = nullptr;
			lv_writes[i+1].pNext = nullptr;
			lv_writes[i+1].pTexelBufferView = nullptr;
			lv_writes[i+1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		}


		vkUpdateDescriptorSets(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device
			, lv_writes.size(), lv_writes.data(), 0, nullptr);

	}

}