




#include "DeferredForwardRenderer.hpp"
#include <format>
#include "UtilsVulkan.h"


namespace RenderCore
{

	DeferredForwardRenderer::DeferredForwardRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator,
		const char* l_vertexShaderPath, const char* l_fragmentShaderPath,
		const char* l_spirvPath)
		:Renderbase(l_vkContextCreator)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();
		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();

		m_samplerTexturesHandles.resize(lv_totalNumSwapchains);

		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {

			auto lv_outputComputeTextureMetaData = lv_vkResManager
				.RetrieveGpuResourceMetaData(std::format(" DeferredRendererOutputImage {} ", i).c_str());

			m_samplerTexturesHandles[i] = lv_outputComputeTextureMetaData.m_resourceHandle;
		}

		GeneratePipelineFromSpirvBinaries(l_spirvPath);
		SetRenderPassAndFrameBuffer("DeferredForward");

		auto* lv_node = lv_frameGraph.RetrieveNode("DeferredForward");

		if (nullptr == lv_node) {
			printf("Indirect renderer was not found among the nodes of the frame graph. Exitting....\n");
			exit(-1);
		}
		lv_node->m_renderer = this;

		UpdateDescriptorSets();

		m_pipelineLayout = lv_vkResManager.CreatePipelineLayout(m_descriptorSetLayout, " DeferredForwardRendererPipelineLayout ");

		VulkanResourceManager::PipelineInfo lv_pipelineInfo{};
		lv_pipelineInfo.m_dynamicScissorState = false;
		lv_pipelineInfo.m_height = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferHeight;
		lv_pipelineInfo.m_width = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferWidth;
		lv_pipelineInfo.m_useBlending = false;
		lv_pipelineInfo.m_useDepth = false;
		lv_pipelineInfo.m_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		lv_pipelineInfo.m_totalNumColorAttach = 1;

		m_graphicsPipeline = lv_vkResManager.CreateGraphicsPipeline(m_renderPass, m_pipelineLayout,
			{ l_vertexShaderPath, l_fragmentShaderPath }, " DeferredRendererPipeline ", lv_pipelineInfo);

	}



	void DeferredForwardRenderer::UpdateBuffers(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure)
	{

	}


	void DeferredForwardRenderer::FillCommandBuffer(VkCommandBuffer l_commandBuffer,
		uint32_t l_currentSwapchainIndex)
	{
		auto lv_framebuffer = m_vulkanRenderContext.GetResourceManager()
			.RetrieveGpuFramebuffer(m_framebufferHandles[l_currentSwapchainIndex]);

		auto& lv_samplerTexture = m_vulkanRenderContext.GetResourceManager().RetrieveGpuTexture(m_samplerTexturesHandles[l_currentSwapchainIndex]);

		/*VkMemoryBarrier lv_memBarrier{};
		lv_memBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		lv_memBarrier.pNext = nullptr;
		lv_memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		lv_memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

		vkCmdPipelineBarrier(l_commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 1,
			&lv_memBarrier, 0, nullptr, 0, nullptr);*/

		transitionImageLayoutCmd(l_commandBuffer, lv_samplerTexture.image.image,
			lv_samplerTexture.format, lv_samplerTexture.Layout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		lv_samplerTexture.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		BeginRenderPass(m_renderPass, lv_framebuffer, l_commandBuffer, l_currentSwapchainIndex, 1);
		vkCmdDraw(l_commandBuffer, 6, 1, 0, 0);
		vkCmdEndRenderPass(l_commandBuffer);


		transitionImageLayoutCmd(l_commandBuffer, lv_samplerTexture.image.image,
			lv_samplerTexture.format, lv_samplerTexture.Layout, VK_IMAGE_LAYOUT_GENERAL);
		lv_samplerTexture.Layout = VK_IMAGE_LAYOUT_GENERAL;

	}


	void DeferredForwardRenderer::UpdateDescriptorSets()
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();


		std::vector<VkWriteDescriptorSet> lv_writes;
		std::vector<VkDescriptorImageInfo> lv_imageInfo;
		lv_imageInfo.resize(lv_totalNumSwapchains);
		lv_writes.resize(lv_totalNumSwapchains);


		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
			
			auto& lv_outputImageCompute = lv_vkResManager.RetrieveGpuTexture(m_samplerTexturesHandles[i]);

			lv_imageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfo[i].imageView = lv_outputImageCompute.image.imageView;
			lv_imageInfo[i].sampler = lv_outputImageCompute.sampler;

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

		vkUpdateDescriptorSets(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device,
			lv_writes.size(), lv_writes.data(), 0, nullptr);

	}
}