



#include "SSAORenderer.hpp"
#include "CameraStructure.hpp"
#include <random>
#include <array>


namespace RenderCore
{


	SSAORenderer::SSAORenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator
		, const char* l_vtxShader
		, const char* l_fragShader
		, const char* l_spvPath)
		
		:Renderbase(l_vkContextCreator)
	{

		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();
		auto lv_totalNumSwapChains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();

		std::uniform_real_distribution<float> lv_randomFloats(0.f, 1.f);
		std::default_random_engine lv_randomEngine;

		constexpr uint32_t lv_sizeArrayOffsets = 64;
		UniformBufferSSAOOffsets lv_offsets;


		//Make sure l_a is smaller than l_b 
		auto Lerp = [](float l_a, float l_b, float l_x) -> float
			{
				return (1.f - l_x) * l_a + l_x * l_b;
			};

		for (size_t i = 0; i < lv_sizeArrayOffsets; ++i) {

			lv_offsets.m_offsets[i].x = lv_randomFloats(lv_randomEngine) * 2.f - 1.f;
			lv_offsets.m_offsets[i].y = lv_randomFloats(lv_randomEngine) * 2.f - 1.f;
			lv_offsets.m_offsets[i].z = lv_randomFloats(lv_randomEngine);
			lv_offsets.m_offsets[i].w = 0.f;


			lv_offsets.m_offsets[i] = glm::normalize(lv_offsets.m_offsets[i]);
			lv_offsets.m_offsets[i] *= lv_randomFloats(lv_randomEngine);


			float lv_downScale = 1.f / 64.f;
			lv_downScale = Lerp(0.1f, 1.f, lv_downScale * lv_downScale);
			lv_offsets.m_offsets[i] *= lv_downScale;
		}


		m_gpuOffsetsHandle = lv_vkResManager.CreateBufferWithHandle(sizeof(lv_offsets)
											,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
											,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
											, "SSAOOffsetsBuffer");
		lv_vkResManager.CopyDataToLocalBuffer(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_mainQueue2,
			m_vulkanRenderContext.GetContextCreator().m_vkDev.m_mainCommandBuffer1[0]
			, m_gpuOffsetsHandle, lv_offsets.m_offsets);


		std::array<glm::vec4, 16> lv_randomRotations{};
		for (size_t i = 0; i < lv_randomRotations.size(); ++i) {
			lv_randomRotations[i].x = lv_randomFloats(lv_randomEngine) * 2.f - 1.f;
			lv_randomRotations[i].y = lv_randomFloats(lv_randomEngine) * 2.f - 1.f;
			lv_randomRotations[i].z = 0.f;
			lv_randomRotations[i].w = 0.f;
		}

		m_gpuRandomRotationsTextureHandle = lv_vkResManager.CreateTexture(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_maxAnisotropy, "RandomRotationsSSAO", VK_FORMAT_R32G32B32A32_SFLOAT, 4, 4);
		auto& lv_randomRotationGpuTexture = lv_vkResManager.RetrieveGpuTexture(m_gpuRandomRotationsTextureHandle);

		assert(true == updateTextureImage(m_vulkanRenderContext.GetContextCreator().m_vkDev
			, lv_randomRotationGpuTexture.image.image, lv_randomRotationGpuTexture.image.imageMemory
			, 4, 4, lv_randomRotationGpuTexture.format, 1, lv_randomRotations.data(), lv_randomRotationGpuTexture.Layout));


		transitionImageLayout(m_vulkanRenderContext.GetContextCreator().m_vkDev, lv_randomRotationGpuTexture.image.image, lv_randomRotationGpuTexture.format, lv_randomRotationGpuTexture.Layout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		lv_randomRotationGpuTexture.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		m_gpuUniformBufferHandle = lv_vkResManager.CreateBufferWithHandle(sizeof(UniformBufferMatrices)
												   , VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
												   , VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
												   , "UniformBufferMatricesSSAO");

		
		GeneratePipelineFromSpirvBinaries(l_spvPath);
		SetRenderPassAndFrameBuffer("SSAO");
		SetNodeToAppropriateRenderpass("SSAO", this);
		UpdateDescriptorSets();
		auto* lv_node = lv_frameGraph.RetrieveNode("SSAO");
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
			, { l_vtxShader, l_fragShader }, "SSAOGraphicsPipeline", lv_pipeInfo);



	}



	void SSAORenderer::UpdateDescriptorSets()
	{

		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();

		auto& lv_offsetBufferGpu = lv_vkResManager.RetrieveGpuBuffer(m_gpuOffsetsHandle);
		auto& lv_uniformBufferGpu = lv_vkResManager.RetrieveGpuBuffer(m_gpuUniformBufferHandle);

		std::array<VkDescriptorBufferInfo, 2> lv_bufferDescriptors{};

		lv_bufferDescriptors[0].buffer = lv_uniformBufferGpu.buffer;
		lv_bufferDescriptors[0].offset = 0;
		lv_bufferDescriptors[0].range = VK_WHOLE_SIZE;

		lv_bufferDescriptors[1].buffer = lv_offsetBufferGpu.buffer;
		lv_bufferDescriptors[1].offset = 0;
		lv_bufferDescriptors[1].range = VK_WHOLE_SIZE;


		std::vector<VkDescriptorImageInfo> lv_imageDescriptors{};
		lv_imageDescriptors.resize(lv_totalNumSwapchains * 3);


		auto& lv_randomRotationTexture = lv_vkResManager.RetrieveGpuTexture(m_gpuRandomRotationsTextureHandle);
		for (size_t i = 0, j = 0; i < lv_imageDescriptors.size(); i += 3, ++j) {

			auto& lv_gpuPosTexture = lv_vkResManager.RetrieveGpuTexture("GBufferPosition", (uint32_t)j);
			auto& lv_gpuNormalVertexTexture = lv_vkResManager.RetrieveGpuTexture("GBufferNormalVertex", (uint32_t)j);

			lv_imageDescriptors[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageDescriptors[i].imageView = lv_gpuPosTexture.image.imageView0;
			lv_imageDescriptors[i].sampler = lv_gpuPosTexture.sampler;

			lv_imageDescriptors[i + 1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageDescriptors[i + 1].imageView = lv_gpuNormalVertexTexture.image.imageView0;
			lv_imageDescriptors[i + 1].sampler = lv_gpuNormalVertexTexture.sampler;

			lv_imageDescriptors[i + 2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageDescriptors[i + 2].imageView = lv_randomRotationTexture.image.imageView0;
			lv_imageDescriptors[i + 2].sampler = lv_randomRotationTexture.sampler;


		}

		std::vector<VkWriteDescriptorSet> lv_writes{};
		lv_writes.resize(5 * lv_totalNumSwapchains);

		for (size_t i = 0, j = 0; i < lv_writes.size(); i += 5, ++j) {

			lv_writes[i].descriptorCount = 1;
			lv_writes[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			lv_writes[i].dstArrayElement = 0;
			lv_writes[i].dstBinding = 0;
			lv_writes[i].dstSet = m_descriptorSets[j];
			lv_writes[i].pBufferInfo = &lv_bufferDescriptors[0];
			lv_writes[i].pImageInfo = nullptr;
			lv_writes[i].pNext = nullptr;
			lv_writes[i].pTexelBufferView = nullptr;
			lv_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i+1].descriptorCount = 1;
			lv_writes[i+1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			lv_writes[i+1].dstArrayElement = 0;
			lv_writes[i+1].dstBinding = 1;
			lv_writes[i+1].dstSet = m_descriptorSets[j];
			lv_writes[i+1].pBufferInfo = &lv_bufferDescriptors[1];
			lv_writes[i+1].pImageInfo = nullptr;
			lv_writes[i+1].pNext = nullptr;
			lv_writes[i+1].pTexelBufferView = nullptr;
			lv_writes[i+1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;


			lv_writes[i + 2].descriptorCount = 1;
			lv_writes[i + 2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 2].dstArrayElement = 0;
			lv_writes[i + 2].dstBinding = 2;
			lv_writes[i + 2].dstSet = m_descriptorSets[j];
			lv_writes[i + 2].pBufferInfo = nullptr;
			lv_writes[i + 2].pImageInfo = &lv_imageDescriptors[3*j];
			lv_writes[i + 2].pNext = nullptr;
			lv_writes[i + 2].pTexelBufferView = nullptr;
			lv_writes[i + 2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 3].descriptorCount = 1;
			lv_writes[i + 3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 3].dstArrayElement = 0;
			lv_writes[i + 3].dstBinding = 3;
			lv_writes[i + 3].dstSet = m_descriptorSets[j];
			lv_writes[i + 3].pBufferInfo = nullptr;
			lv_writes[i + 3].pImageInfo = &lv_imageDescriptors[3 * j + 1];
			lv_writes[i + 3].pNext = nullptr;
			lv_writes[i + 3].pTexelBufferView = nullptr;
			lv_writes[i + 3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 4].descriptorCount = 1;
			lv_writes[i + 4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 4].dstArrayElement = 0;
			lv_writes[i + 4].dstBinding = 4;
			lv_writes[i + 4].dstSet = m_descriptorSets[j];
			lv_writes[i + 4].pBufferInfo = nullptr;
			lv_writes[i + 4].pImageInfo = &lv_imageDescriptors[3 * j + 2];
			lv_writes[i + 4].pNext = nullptr;
			lv_writes[i + 4].pTexelBufferView = nullptr;
			lv_writes[i + 4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		}

		vkUpdateDescriptorSets(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device, lv_writes.size(), lv_writes.data(), 0, nullptr);

	}



	void SSAORenderer::UpdateBuffers(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();

		UniformBufferMatrices lv_uniformBuffer;
		lv_uniformBuffer.m_projectionMatrix = l_cameraStructure.m_projectionMatrix;
		lv_uniformBuffer.m_viewMatrix = l_cameraStructure.m_viewMatrix;

		memcpy(lv_vkResManager.RetrieveGpuBuffer(m_gpuUniformBufferHandle).ptr, &lv_uniformBuffer, sizeof(UniformBufferMatrices));
	}

	void SSAORenderer::FillCommandBuffer(VkCommandBuffer l_cmdBuffer, uint32_t l_currentSwapchainIndex)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();


		auto lv_framebuffer = lv_vkResManager.RetrieveGpuFramebuffer(m_framebufferHandles[l_currentSwapchainIndex]);
		
		auto& lv_gpuOcclusionTexture = lv_vkResManager.RetrieveGpuTexture("OcclusionFactor", l_currentSwapchainIndex);

		
		transitionImageLayoutCmd(l_cmdBuffer, lv_gpuOcclusionTexture.image.image
			, lv_gpuOcclusionTexture.format, lv_gpuOcclusionTexture.Layout
			, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		
		lv_gpuOcclusionTexture.Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		BeginRenderPass(m_renderPass, lv_framebuffer, l_cmdBuffer, l_currentSwapchainIndex, 1);
		vkCmdDraw(l_cmdBuffer, 6, 1, 0, 0);
		vkCmdEndRenderPass(l_cmdBuffer);

		lv_gpuOcclusionTexture.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	}

}