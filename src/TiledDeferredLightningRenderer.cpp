


#include "TiledDeferredLightningRenderer.hpp"
#include <format>
#include "CameraStructure.hpp"
#include <GLFW/glfw3.h>

namespace RenderCore
{


	TiledDeferredLightningRenderer::TiledDeferredLightningRenderer
	(VulkanEngine::VulkanRenderContext& l_vkContextCreator
		, const char* l_computeShader
		, const char* l_spvPath)
		:Renderbase(l_vkContextCreator)
	{

		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();

		std::array<Light, m_totalNumLights> lv_lightData;
		std::array<glm::vec4, m_totalNumLights> lv_positionData;
		std::array<glm::vec4, 8> lv_unitCubeData
		{
			glm::vec4{-1.0f, -1.0f, -1.0f, 0.f},
			glm::vec4{ 1.0f, -1.0f, -1.0f, 0.f},
			glm::vec4{ 1.0f,  1.0f, -1.0f, 0.f},
			glm::vec4{-1.0f,  1.0f, -1.0f, 0.f},
			glm::vec4{-1.0f, -1.0f,  1.0f, 0.f},
			glm::vec4{ 1.0f, -1.0f,  1.0f, 0.f},
			glm::vec4{ 1.0f,  1.0f,  1.0f, 0.f},
			glm::vec4{-1.0f,  1.0f,  1.0f, 0.f}
		};




		InitializePositionData(lv_positionData);
		InitializeLightBuffer(lv_lightData, lv_positionData);

		m_lightBufferGpuHandle = lv_vkResManager.CreateBufferWithHandle
		(sizeof(Light) * m_totalNumLights, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
			, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			, "TiledLightStorageBufferDeferredRenderpass");
		auto& lv_lightGpu = lv_vkResManager.RetrieveGpuBuffer(m_lightBufferGpuHandle);

		lv_vkResManager.CopyDataToLocalBuffer(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_mainQueue2,
			m_vulkanRenderContext.GetContextCreator().m_vkDev.m_mainCommandBuffer1[0]
			, m_lightBufferGpuHandle, lv_lightData.data());

		m_uniformBufferGpuHandle = lv_vkResManager.CreateBufferWithHandle
		(sizeof(UniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
			, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			, "TiledUniformBufferDeferredRenderpass");

		m_debugBuffer = &lv_vkResManager.CreateBuffer(sizeof(float) * 44 * 44, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
			, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			, "TiledDebugBufferDeferredLightning");



		m_colorOutputTextures.resize(lv_totalNumSwapchains);

		auto lv_depthMapLightMeta = lv_vkResManager.RetrieveGpuResourceMetaData("DepthMapPointLight");

		assert(lv_depthMapLightMeta.m_resourceHandle != std::numeric_limits<uint32_t>::max());

		m_depthMapLightGpuHandle = lv_depthMapLightMeta.m_resourceHandle;

		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
			m_colorOutputTextures[i] = &lv_vkResManager.RetrieveGpuTexture("DeferredLightningColorTexture", i);
		}


		GeneratePipelineFromSpirvBinaries(l_spvPath);
		SetNodeToAppropriateRenderpass("TiledDeferredLightning", this);
		UpdateDescriptorSets();

		auto* lv_node = lv_frameGraph.RetrieveNode("TiledDeferredLightning");
		lv_node->m_enabled = false;
		lv_frameGraph.IncrementNumNodesPerCmdBuffer(2);

		m_computePipeline = lv_vkResManager.CreateComputePipeline
		(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device
			, l_computeShader, m_pipelineLayout);

		m_debugComputePipeline = lv_vkResManager.CreateComputePipeline
		(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device
			,"Shaders/DebugFindingMaxMinDepthOfEachTile.comp.comp", m_pipelineLayout);

	}



	void TiledDeferredLightningRenderer::SetSwitchToDebugTiled(bool l_switch)
	{
		m_switchToDebug = l_switch;
	}

	void TiledDeferredLightningRenderer::UpdateDescriptorSets()
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();

		std::array<VkDescriptorBufferInfo, 3> lv_bufferInfos{};
		std::vector<VkDescriptorImageInfo> lv_imageInfos{};
		lv_imageInfos.resize(lv_totalNumSwapchains * 10);


		auto& lv_lightBufferGpu = lv_vkResManager.RetrieveGpuBuffer(m_lightBufferGpuHandle);
		auto& lv_uniformBufferGpu = lv_vkResManager.RetrieveGpuBuffer(m_uniformBufferGpuHandle);

		lv_bufferInfos[0].buffer = lv_lightBufferGpu.buffer;
		lv_bufferInfos[0].offset = 0;
		lv_bufferInfos[0].range = VK_WHOLE_SIZE;

		lv_bufferInfos[1].buffer = lv_uniformBufferGpu.buffer;
		lv_bufferInfos[1].offset = 0;
		lv_bufferInfos[1].range = VK_WHOLE_SIZE;

		lv_bufferInfos[2].buffer = m_debugBuffer->buffer;
		lv_bufferInfos[2].offset = 0;
		lv_bufferInfos[2].range = VK_WHOLE_SIZE;



		auto lv_depthMapLightCubeMeta = lv_vkResManager.RetrieveGpuResourceMetaData("DepthMapPointLight");
		assert(std::numeric_limits<uint32_t>::max() != lv_depthMapLightCubeMeta.m_resourceHandle);
		auto& lv_depthMapLightGpu = lv_vkResManager.RetrieveGpuTexture(lv_depthMapLightCubeMeta.m_resourceHandle);
		for (size_t i = 0, j = 0; i < lv_imageInfos.size(); i += 10, ++j) {

			auto& lv_gbufferPosGpu = lv_vkResManager.RetrieveGpuTexture("GBufferPosition", j);
			auto& lv_gbufferNormalGpu = lv_vkResManager.RetrieveGpuTexture("GBufferNormal", j);
			auto& lv_gbufferAlbedoSpec = lv_vkResManager.RetrieveGpuTexture("GBufferAlbedoSpec", j);
			auto& lv_gbufferTangent = lv_vkResManager.RetrieveGpuTexture("GBufferTangent", j);
			auto& lv_gbufferNormalVertexGpu = lv_vkResManager.RetrieveGpuTexture("GBufferNormalVertex", j);
			auto& lv_occlusionGpu = lv_vkResManager.RetrieveGpuTexture("BoxBlurTexture", j);
			auto& lv_gbufferMetallicGpu = lv_vkResManager.RetrieveGpuTexture("GBufferMetallic", j);
			auto& lv_depth = lv_vkResManager.RetrieveGpuTexture("Depth", j);

			lv_imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfos[i].imageView = lv_gbufferPosGpu.image.imageView0;
			lv_imageInfos[i].sampler = lv_gbufferPosGpu.sampler;

			lv_imageInfos[i + 1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfos[i + 1].imageView = lv_gbufferNormalGpu.image.imageView0;
			lv_imageInfos[i + 1].sampler = lv_gbufferNormalGpu.sampler;

			lv_imageInfos[i + 2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfos[i + 2].imageView = lv_gbufferAlbedoSpec.image.imageView0;
			lv_imageInfos[i + 2].sampler = lv_gbufferAlbedoSpec.sampler;

			lv_imageInfos[i + 3].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfos[i + 3].imageView = lv_gbufferTangent.image.imageView0;
			lv_imageInfos[i + 3].sampler = lv_gbufferTangent.sampler;

			lv_imageInfos[i + 4].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfos[i + 4].imageView = lv_gbufferNormalVertexGpu.image.imageView0;
			lv_imageInfos[i + 4].sampler = lv_gbufferNormalVertexGpu.sampler;

			lv_imageInfos[i + 5].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfos[i + 5].imageView = lv_occlusionGpu.image.imageView0;
			lv_imageInfos[i + 5].sampler = lv_occlusionGpu.sampler;

			lv_imageInfos[i + 6].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfos[i + 6].imageView = lv_gbufferMetallicGpu.image.imageView0;
			lv_imageInfos[i + 6].sampler = lv_gbufferMetallicGpu.sampler;

			lv_imageInfos[i + 7].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfos[i + 7].imageView = lv_depthMapLightGpu.image.cubemapImageView;
			lv_imageInfos[i + 7].sampler = lv_depthMapLightGpu.sampler;

			lv_imageInfos[i + 8].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfos[i + 8].imageView = lv_depth.image.imageView0;
			lv_imageInfos[i + 8].sampler = lv_depth.sampler;

			lv_imageInfos[i + 9].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			lv_imageInfos[i + 9].imageView = m_colorOutputTextures[j]->image.imageView0;
			lv_imageInfos[i + 9].sampler = m_colorOutputTextures[j]->sampler;

		}

		std::vector<VkWriteDescriptorSet> lv_writes;
		lv_writes.resize(lv_totalNumSwapchains * lv_bufferInfos.size() + lv_imageInfos.size());


		for (size_t i = 0, j = 0; i < lv_writes.size(); i += 13, ++j) {

			lv_writes[i].descriptorCount = 1;
			lv_writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			lv_writes[i].dstArrayElement = 0;
			lv_writes[i].dstBinding = 0;
			lv_writes[i].dstSet = m_descriptorSets[j];
			lv_writes[i].pBufferInfo = &lv_bufferInfos[0];
			lv_writes[i].pImageInfo = nullptr;
			lv_writes[i].pNext = nullptr;
			lv_writes[i].pTexelBufferView = nullptr;
			lv_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 1].descriptorCount = 1;
			lv_writes[i + 1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			lv_writes[i + 1].dstArrayElement = 0;
			lv_writes[i + 1].dstBinding = 1;
			lv_writes[i + 1].dstSet = m_descriptorSets[j];
			lv_writes[i + 1].pBufferInfo = &lv_bufferInfos[1];
			lv_writes[i + 1].pImageInfo = nullptr;
			lv_writes[i + 1].pNext = nullptr;
			lv_writes[i + 1].pTexelBufferView = nullptr;
			lv_writes[i + 1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;



			lv_writes[i + 2].descriptorCount = 1;
			lv_writes[i + 2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 2].dstArrayElement = 0;
			lv_writes[i + 2].dstBinding = 2;
			lv_writes[i + 2].dstSet = m_descriptorSets[j];
			lv_writes[i + 2].pBufferInfo = nullptr;
			lv_writes[i + 2].pImageInfo = &lv_imageInfos[10 * j];
			lv_writes[i + 2].pNext = nullptr;
			lv_writes[i + 2].pTexelBufferView = nullptr;
			lv_writes[i + 2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 3].descriptorCount = 1;
			lv_writes[i + 3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 3].dstArrayElement = 0;
			lv_writes[i + 3].dstBinding = 3;
			lv_writes[i + 3].dstSet = m_descriptorSets[j];
			lv_writes[i + 3].pBufferInfo = nullptr;
			lv_writes[i + 3].pImageInfo = &lv_imageInfos[10 * j + 1];
			lv_writes[i + 3].pNext = nullptr;
			lv_writes[i + 3].pTexelBufferView = nullptr;
			lv_writes[i + 3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 4].descriptorCount = 1;
			lv_writes[i + 4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 4].dstArrayElement = 0;
			lv_writes[i + 4].dstBinding = 4;
			lv_writes[i + 4].dstSet = m_descriptorSets[j];
			lv_writes[i + 4].pBufferInfo = nullptr;
			lv_writes[i + 4].pImageInfo = &lv_imageInfos[10 * j + 2];
			lv_writes[i + 4].pNext = nullptr;
			lv_writes[i + 4].pTexelBufferView = nullptr;
			lv_writes[i + 4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 5].descriptorCount = 1;
			lv_writes[i + 5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 5].dstArrayElement = 0;
			lv_writes[i + 5].dstBinding = 5;
			lv_writes[i + 5].dstSet = m_descriptorSets[j];
			lv_writes[i + 5].pBufferInfo = nullptr;
			lv_writes[i + 5].pImageInfo = &lv_imageInfos[10 * j + 3];
			lv_writes[i + 5].pNext = nullptr;
			lv_writes[i + 5].pTexelBufferView = nullptr;
			lv_writes[i + 5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 6].descriptorCount = 1;
			lv_writes[i + 6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 6].dstArrayElement = 0;
			lv_writes[i + 6].dstBinding = 6;
			lv_writes[i + 6].dstSet = m_descriptorSets[j];
			lv_writes[i + 6].pBufferInfo = nullptr;
			lv_writes[i + 6].pImageInfo = &lv_imageInfos[10 * j + 4];
			lv_writes[i + 6].pNext = nullptr;
			lv_writes[i + 6].pTexelBufferView = nullptr;
			lv_writes[i + 6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 7].descriptorCount = 1;
			lv_writes[i + 7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 7].dstArrayElement = 0;
			lv_writes[i + 7].dstBinding = 7;
			lv_writes[i + 7].dstSet = m_descriptorSets[j];
			lv_writes[i + 7].pBufferInfo = nullptr;
			lv_writes[i + 7].pImageInfo = &lv_imageInfos[10 * j + 5];
			lv_writes[i + 7].pNext = nullptr;
			lv_writes[i + 7].pTexelBufferView = nullptr;
			lv_writes[i + 7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 8].descriptorCount = 1;
			lv_writes[i + 8].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 8].dstArrayElement = 0;
			lv_writes[i + 8].dstBinding = 8;
			lv_writes[i + 8].dstSet = m_descriptorSets[j];
			lv_writes[i + 8].pBufferInfo = nullptr;
			lv_writes[i + 8].pImageInfo = &lv_imageInfos[10 * j + 6];
			lv_writes[i + 8].pNext = nullptr;
			lv_writes[i + 8].pTexelBufferView = nullptr;
			lv_writes[i + 8].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 9].descriptorCount = 1;
			lv_writes[i + 9].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 9].dstArrayElement = 0;
			lv_writes[i + 9].dstBinding = 9;
			lv_writes[i + 9].dstSet = m_descriptorSets[j];
			lv_writes[i + 9].pBufferInfo = nullptr;
			lv_writes[i + 9].pImageInfo = &lv_imageInfos[10 * j + 7];
			lv_writes[i + 9].pNext = nullptr;
			lv_writes[i + 9].pTexelBufferView = nullptr;
			lv_writes[i + 9].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;


			lv_writes[i + 10].descriptorCount = 1;
			lv_writes[i + 10].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 10].dstArrayElement = 0;
			lv_writes[i + 10].dstBinding = 10;
			lv_writes[i + 10].dstSet = m_descriptorSets[j];
			lv_writes[i + 10].pBufferInfo = nullptr;
			lv_writes[i + 10].pImageInfo = &lv_imageInfos[10 * j + 8];
			lv_writes[i + 10].pNext = nullptr;
			lv_writes[i + 10].pTexelBufferView = nullptr;
			lv_writes[i + 10].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 11].descriptorCount = 1;
			lv_writes[i + 11].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			lv_writes[i + 11].dstArrayElement = 0;
			lv_writes[i + 11].dstBinding = 11;
			lv_writes[i + 11].dstSet = m_descriptorSets[j];
			lv_writes[i + 11].pBufferInfo = nullptr;
			lv_writes[i + 11].pImageInfo = &lv_imageInfos[10 * j + 9];
			lv_writes[i + 11].pNext = nullptr;
			lv_writes[i + 11].pTexelBufferView = nullptr;
			lv_writes[i + 11].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 12].descriptorCount = 1;
			lv_writes[i + 12].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			lv_writes[i + 12].dstArrayElement = 0;
			lv_writes[i + 12].dstBinding = 12;
			lv_writes[i + 12].dstSet = m_descriptorSets[j];
			lv_writes[i + 12].pBufferInfo = &lv_bufferInfos[2];
			lv_writes[i + 12].pImageInfo = nullptr;
			lv_writes[i + 12].pNext = nullptr;
			lv_writes[i + 12].pTexelBufferView = nullptr;
			lv_writes[i + 12].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		}

		vkUpdateDescriptorSets(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device
			, lv_writes.size(), lv_writes.data(), 0, nullptr);
	}


	void TiledDeferredLightningRenderer::UpdateBuffers
	(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure)
	{
		UniformBuffer lv_cameraUniform{};
		lv_cameraUniform.m_cameraPos = glm::vec4{ l_cameraStructure.m_cameraPos, 1.f };
		lv_cameraUniform.m_viewMatrix = l_cameraStructure.m_viewMatrix;
		lv_cameraUniform.m_inMtx = l_cameraStructure.m_projectionMatrix * l_cameraStructure.m_viewMatrix;
		lv_cameraUniform.m_invProjMatrix = glm::inverse(l_cameraStructure.m_projectionMatrix);
		lv_cameraUniform.m_projMatrix = l_cameraStructure.m_projectionMatrix;

		auto& lv_uniformBufferGpu = m_vulkanRenderContext.GetResourceManager().RetrieveGpuBuffer(m_uniformBufferGpuHandle);
		memcpy(lv_uniformBufferGpu.ptr, &lv_cameraUniform, lv_uniformBufferGpu.size);

	}

	void TiledDeferredLightningRenderer::FillCommandBuffer
	(VkCommandBuffer l_cmdBuffer, uint32_t l_currentSwapchainIndex)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();

		auto& lv_gbufferTangentGpu = lv_vkResManager.RetrieveGpuTexture("GBufferTangent", l_currentSwapchainIndex);
		auto& lv_gbufferPosGpu = lv_vkResManager.RetrieveGpuTexture("GBufferPosition", l_currentSwapchainIndex);
		auto& lv_gbufferNormalGpu = lv_vkResManager.RetrieveGpuTexture("GBufferNormal", l_currentSwapchainIndex);
		auto& lv_gbufferAlbedoSpecGpu = lv_vkResManager.RetrieveGpuTexture("GBufferAlbedoSpec", l_currentSwapchainIndex);
		auto& lv_gbufferNormalVertexGpu = lv_vkResManager.RetrieveGpuTexture("GBufferNormalVertex", l_currentSwapchainIndex);
		auto& lv_depth = lv_vkResManager.RetrieveGpuTexture("Depth", l_currentSwapchainIndex);
		auto& lv_metallicGpu = lv_vkResManager.RetrieveGpuTexture("GBufferMetallic", l_currentSwapchainIndex);




		transitionImageLayoutCmd(l_cmdBuffer, m_colorOutputTextures[l_currentSwapchainIndex]->image.image
			, m_colorOutputTextures[l_currentSwapchainIndex]->format, m_colorOutputTextures[l_currentSwapchainIndex]->Layout
			, VK_IMAGE_LAYOUT_GENERAL);



		if (true == m_switchToDebug) {
			vkCmdBindPipeline(l_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_debugComputePipeline);
		}
		else {
			vkCmdBindPipeline(l_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);
		}
		vkCmdBindDescriptorSets(l_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1,
			&m_descriptorSets[l_currentSwapchainIndex], 0, nullptr);

		uint32_t lv_width = 1024;
		uint32_t lv_height = 1024;


		vkCmdDispatch(l_cmdBuffer, (uint32_t)lv_width / (uint32_t)16, (uint32_t)lv_height / (uint32_t)16, 1);

		transitionImageLayoutCmd(l_cmdBuffer, m_colorOutputTextures[l_currentSwapchainIndex]->image.image
			, m_colorOutputTextures[l_currentSwapchainIndex]->format, VK_IMAGE_LAYOUT_GENERAL
			, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		m_colorOutputTextures[l_currentSwapchainIndex]->Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;



	}

	void TiledDeferredLightningRenderer::InitializePositionData
	(std::array<glm::vec4, m_totalNumLights>& l_positionData)
	{
		l_positionData[0] = glm::vec4{ -13.f, 18.f, -2.f, 1.f };


		for (uint32_t i = 1; i < 8; ++i) {
			l_positionData[i] = glm::vec4{ -45.f + (float)20 * i, 0.5f, 35.f, (float)i };
		}


		for (uint32_t j = 0; j < 6; ++j) {
			for (uint32_t i = 0; i < 8; ++i) {
				l_positionData[8 * j + i + 8] = glm::vec4{ -65.f + (float)17 * i, 0.5f, -27.5f + (float)(13.5f * j), (float)i };
			}
		}

		for (uint32_t j = 0; j < 6; ++j) {

			if (j == 0) {
				for (uint32_t i = 0; i < 8; ++i) {
					l_positionData[8 * j + i + 56] = glm::vec4{ -65.f + (float)17 * i, 15.f, -27.5f, (float)i };
				}
			}

			if (j == 1) {
				for (uint32_t i = 0; i < 8; ++i) {
					l_positionData[8 * j + i + 56] = glm::vec4{ -65.f + (float)17 * i, 15.f, 21.5f, (float)i };
				}
			}


			if (j == 3) {
				for (uint32_t i = 0; i < 8; ++i) {
					l_positionData[8 * j + i + 56] = glm::vec4{ -65.f + (float)17 * i, 23.f, -23.5f, (float)i };
				}
			}

			if (j == 2) {
				for (uint32_t i = 0; i < 8; ++i) {
					l_positionData[8 * j + i + 56] = glm::vec4{ -65.f + (float)17 * i, 23.f, -8.f, (float)i };
				}

			}

			if (j == 4) {
				for (uint32_t i = 0; i < 8; ++i) {
					l_positionData[8 * j + i + 56] = glm::vec4{ -65.f + (float)17 * i, 23.f, 8.f, (float)i };
				}
			}

			if (j == 5) {
				for (uint32_t i = 0; i < 8; ++i) {
					l_positionData[8 * j + i + 56] = glm::vec4{ -65.f + (float)17 * i, 26.5f, 22.5f, (float)i };
				}
			}



		}


		for (uint32_t j = 0; j < 16; ++j) {


			if (j == 0) {
				for (uint32_t i = 0; i < 8; ++i) {

					l_positionData[8 * j + i + 104] = glm::vec4{ -65.f + (float)17 * i, 30.f, -27.5f, (float)i };
				}
			}


			if (j == 1) {
				for (uint32_t i = 0; i < 8; ++i) {
					l_positionData[8 * j + i + 104] = glm::vec4{ -65.f + (float)17 * i, 30.f, 24.5f, (float)i };

				}
			}

			if (j == 2) {
				for (uint32_t i = 0; i < 8; ++i) {
					l_positionData[8 * j + i + 104] = glm::vec4{ -65.f + (float)17 * i, 36.f, 20.5f, (float)i };

				}
			}

			if (j == 3) {
				for (uint32_t i = 0; i < 8; ++i) {
					l_positionData[8 * j + i + 104] = glm::vec4{ -65.f + (float)17 * i, 36.f, -23.5f, (float)i };

				}
			}



			//Starts from here
			if (j == 4) {
				for (uint32_t i = 0; i < 8; ++i) {
					l_positionData[8 * j + i + 104] = glm::vec4{ -65.f + (float)17 * i, 50.f, -5.2f, (float)i };

				}
			}
			if (j == 5) {
				for (uint32_t i = 0; i < 8; ++i) {
					l_positionData[8 * j + i + 104] = glm::vec4{ -65.f + (float)17 * i, 50.f, 3.5f, (float)i };

				}
			}


			if (j == 6) {
				for (uint32_t i = 0; i < 8; ++i) {
					l_positionData[8 * j + i + 104] = glm::vec4{ -65.f + (float)17 * i, 32.f, -5.2f, (float)i };

				}
			}
			if (j == 7) {
				for (uint32_t i = 0; i < 8; ++i) {
					l_positionData[8 * j + i + 104] = glm::vec4{ -65.f + (float)21 * i, 6.f, 9.f, (float)i };

				}
			}
			if (j == 8) {
				for (uint32_t i = 0; i < 8; ++i) {
					l_positionData[8 * j + i + 104] = glm::vec4{ -65.f + (float)21 * i, 6.f, -9.f, (float)i };

				}
			}
			if (j == 9) {
				for (uint32_t i = 0; i < 8; ++i) {
					l_positionData[8 * j + i + 104] = glm::vec4{ -65.f + (float)17 * i, 36.f, -23.5f, (float)i };

				}
			}
			if (j == 10) {
				for (uint32_t i = 0; i < 8; ++i) {
					l_positionData[8 * j + i + 104] = glm::vec4{ -65.f + (float)17 * i, 25.f, -8.5f, (float)i };

				}
			}
			if (j == 11) {
				for (uint32_t i = 0; i < 8; ++i) {
					l_positionData[8 * j + i + 104] = glm::vec4{ -65.f + (float)17 * i, 25.f, 8.5f, (float)i };

				}
			}
			if (j == 12) {
				for (uint32_t i = 0; i < 8; ++i) {
					l_positionData[8 * j + i + 104] = glm::vec4{ -65.f + (float)17 * i, 44.f, -5.2f, (float)i };

				}
			}
			if (j == 13) {
				for (uint32_t i = 0; i < 8; ++i) {
					l_positionData[8 * j + i + 104] = glm::vec4{ -65.f + (float)17 * i, 44.f, 3.5f, (float)i };

				}
			}
			if (j == 14) {
				for (uint32_t i = 0; i < 8; ++i) {
					l_positionData[8 * j + i + 104] = glm::vec4{ -65.f + (float)17 * i, 38.5f, -5.2f, (float)i };

				}
			}
			if (j == 15) {
				for (uint32_t i = 0; i < 8; ++i) {
					l_positionData[8 * j + i + 104] = glm::vec4{ -65.f + (float)17 * i, 38.5f, 3.5f, (float)i };

				}
			}

		}


	}

	void TiledDeferredLightningRenderer::InitializeLightBuffer
	(std::array<Light, m_totalNumLights>& l_lightBuffer
		, const std::array<glm::vec4, m_totalNumLights>& l_positionData)
	{
		for (uint32_t i = 0; i < m_totalNumLights; ++i) {
			l_lightBuffer[i].m_positionAndRadius = glm::vec4{ l_positionData[i].x,l_positionData[i].y , l_positionData[i].z, 17.f };
		}
	}

}