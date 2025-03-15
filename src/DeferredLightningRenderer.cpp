


#include "DeferredLightningRenderer.hpp"
#include <format>
#include "CameraStructure.hpp"
#include <GLFW/glfw3.h>

namespace RenderCore
{


	DeferredLightningRenderer::DeferredLightningRenderer
	(VulkanEngine::VulkanRenderContext& l_vkContextCreator
		, const char* l_vtxShader
		, const char* l_fragShader
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


		std::array<glm::vec4, 8*m_totalNumLights> lv_vertexBuffer;

		//Clockwise winding order
		std::array<uint16_t, 36> lv_indexBuffer
		{
			0, 1, 2,  2, 3, 0,
			// Right face
			1, 5, 6,  6, 2, 1,
			// Back face
			5, 4, 7,  7, 6, 5,
			// Left face
			4, 0, 3,  3, 7, 4,
			// Top face
			3, 2, 6,  6, 7, 3,
			// Bottom face
			4, 5, 1,  1, 0, 4
		};



		InitializePositionData(lv_positionData);
		InitializeLightBuffer(lv_lightData, lv_positionData);
		InitializeVertexBuffer(lv_positionData, lv_vertexBuffer, lv_unitCubeData);

		m_lightBufferGpuHandle = lv_vkResManager.CreateBufferWithHandle
		(sizeof(Light) * m_totalNumLights, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
			, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			, "LightStorageBufferDeferredRenderpass");
		auto& lv_lightGpu = lv_vkResManager.RetrieveGpuBuffer(m_lightBufferGpuHandle);
		memcpy(lv_lightGpu.ptr, lv_lightData.data(), lv_lightData.size() * sizeof(Light));
		

		m_uniformBufferGpuHandle = lv_vkResManager.CreateBufferWithHandle
		(sizeof(UniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
			, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			, "UniformBufferDeferredRenderpass");
		

	
		
		m_colorOutputTextures.resize(lv_totalNumSwapchains);

		auto lv_depthMapLightMeta = lv_vkResManager.RetrieveGpuResourceMetaData("DepthMapPointLight");

		assert(lv_depthMapLightMeta.m_resourceHandle != std::numeric_limits<uint32_t>::max());

		m_depthMapLightGpuHandle = lv_depthMapLightMeta.m_resourceHandle;

		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
			m_colorOutputTextures[i] = &lv_vkResManager.RetrieveGpuTexture("DeferredLightningColorTexture", i);
		}


		GeneratePipelineFromSpirvBinaries(l_spvPath);
		SetRenderPassAndFrameBuffer("DeferredLightning");
		SetNodeToAppropriateRenderpass("DeferredLightning", this);
		UpdateDescriptorSets();

		auto* lv_node = lv_frameGraph.RetrieveNode("DeferredLightning");
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
			, {l_vtxShader, l_fragShader},"GraphicsPipelineDeferredLightning", lv_pipeInfo);
	}


	void DeferredLightningRenderer::UpdateDescriptorSets()
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();

		std::array<VkDescriptorBufferInfo, 3> lv_bufferInfos{};
		std::vector<VkDescriptorImageInfo> lv_imageInfos{};
		lv_imageInfos.resize(lv_totalNumSwapchains * 9);


		auto& lv_lightBufferGpu = lv_vkResManager.RetrieveGpuBuffer(m_lightBufferGpuHandle);
		auto& lv_uniformBufferGpu = lv_vkResManager.RetrieveGpuBuffer(m_uniformBufferGpuHandle);
		auto lv_uniformBufferSunMeta = lv_vkResManager.RetrieveGpuResourceMetaData("UniformBufferLightMatricesDepthMap");
		auto* lv_uniformBufferSunGpu = &lv_vkResManager.RetrieveGpuBuffer(lv_uniformBufferSunMeta.m_resourceHandle);

		lv_bufferInfos[0].buffer = lv_uniformBufferGpu.buffer;
		lv_bufferInfos[0].offset = 0;
		lv_bufferInfos[0].range = VK_WHOLE_SIZE;

		lv_bufferInfos[1].buffer = lv_lightBufferGpu.buffer;
		lv_bufferInfos[1].offset = 0;
		lv_bufferInfos[1].range = VK_WHOLE_SIZE;

		lv_bufferInfos[2].buffer = lv_uniformBufferSunGpu->buffer;
		lv_bufferInfos[2].offset = 0;
		lv_bufferInfos[2].range = VK_WHOLE_SIZE;


		auto lv_depthMapLightCubeMeta = lv_vkResManager.RetrieveGpuResourceMetaData("DepthMapPointLight");
		assert(std::numeric_limits<uint32_t>::max() != lv_depthMapLightCubeMeta.m_resourceHandle);
		auto& lv_depthMapLightGpu = lv_vkResManager.RetrieveGpuTexture(lv_depthMapLightCubeMeta.m_resourceHandle);
		for (size_t i = 0, j = 0; i < lv_imageInfos.size(); i+=9, ++j) {
			
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

		}

		std::vector<VkWriteDescriptorSet> lv_writes;
		lv_writes.resize(lv_totalNumSwapchains*lv_bufferInfos.size() + lv_imageInfos.size());


		for (size_t i = 0, j = 0; i < lv_writes.size(); i += 12, ++j) {

			lv_writes[i].descriptorCount = 1;
			lv_writes[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ;
			lv_writes[i].dstArrayElement = 0;
			lv_writes[i].dstBinding = 0;
			lv_writes[i].dstSet = m_descriptorSets[j];
			lv_writes[i].pBufferInfo = &lv_bufferInfos[0];
			lv_writes[i].pImageInfo = nullptr;
			lv_writes[i].pNext = nullptr;
			lv_writes[i].pTexelBufferView = nullptr;
			lv_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i+1].descriptorCount = 1;
			lv_writes[i+1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			lv_writes[i+1].dstArrayElement = 0;
			lv_writes[i+1].dstBinding = 1;
			lv_writes[i+1].dstSet = m_descriptorSets[j];
			lv_writes[i+1].pBufferInfo = &lv_bufferInfos[1];
			lv_writes[i+1].pImageInfo = nullptr;
			lv_writes[i+1].pNext = nullptr;
			lv_writes[i+1].pTexelBufferView = nullptr;
			lv_writes[i+1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;



			lv_writes[i+2].descriptorCount = 1;
			lv_writes[i+2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i+2].dstArrayElement = 0;
			lv_writes[i+2].dstBinding = 2;
			lv_writes[i+2].dstSet = m_descriptorSets[j];
			lv_writes[i+2].pBufferInfo = nullptr;
			lv_writes[i+2].pImageInfo = &lv_imageInfos[9*j];
			lv_writes[i+2].pNext = nullptr;
			lv_writes[i+2].pTexelBufferView = nullptr;
			lv_writes[i+2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 3].descriptorCount = 1;
			lv_writes[i + 3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 3].dstArrayElement = 0;
			lv_writes[i + 3].dstBinding = 3;
			lv_writes[i + 3].dstSet = m_descriptorSets[j];
			lv_writes[i + 3].pBufferInfo = nullptr;
			lv_writes[i + 3].pImageInfo = &lv_imageInfos[9 * j + 1];
			lv_writes[i + 3].pNext = nullptr;
			lv_writes[i + 3].pTexelBufferView = nullptr;
			lv_writes[i + 3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 4].descriptorCount = 1;
			lv_writes[i + 4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 4].dstArrayElement = 0;
			lv_writes[i + 4].dstBinding = 4;
			lv_writes[i + 4].dstSet = m_descriptorSets[j];
			lv_writes[i + 4].pBufferInfo = nullptr;
			lv_writes[i + 4].pImageInfo = &lv_imageInfos[9 * j + 2];
			lv_writes[i + 4].pNext = nullptr;
			lv_writes[i + 4].pTexelBufferView = nullptr;
			lv_writes[i + 4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 5].descriptorCount = 1;
			lv_writes[i + 5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 5].dstArrayElement = 0;
			lv_writes[i + 5].dstBinding = 5;
			lv_writes[i + 5].dstSet = m_descriptorSets[j];
			lv_writes[i + 5].pBufferInfo = nullptr;
			lv_writes[i + 5].pImageInfo = &lv_imageInfos[9 * j + 3];
			lv_writes[i + 5].pNext = nullptr;
			lv_writes[i + 5].pTexelBufferView = nullptr;
			lv_writes[i + 5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 6].descriptorCount = 1;
			lv_writes[i + 6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 6].dstArrayElement = 0;
			lv_writes[i + 6].dstBinding = 6;
			lv_writes[i + 6].dstSet = m_descriptorSets[j];
			lv_writes[i + 6].pBufferInfo = nullptr;
			lv_writes[i + 6].pImageInfo = &lv_imageInfos[9 * j + 4];
			lv_writes[i + 6].pNext = nullptr;
			lv_writes[i + 6].pTexelBufferView = nullptr;
			lv_writes[i + 6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 7].descriptorCount = 1;
			lv_writes[i + 7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 7].dstArrayElement = 0;
			lv_writes[i + 7].dstBinding = 7;
			lv_writes[i + 7].dstSet = m_descriptorSets[j];
			lv_writes[i + 7].pBufferInfo = nullptr;
			lv_writes[i + 7].pImageInfo = &lv_imageInfos[9 * j + 5];
			lv_writes[i + 7].pNext = nullptr;
			lv_writes[i + 7].pTexelBufferView = nullptr;
			lv_writes[i + 7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 8].descriptorCount = 1;
			lv_writes[i + 8].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 8].dstArrayElement = 0;
			lv_writes[i + 8].dstBinding = 8;
			lv_writes[i + 8].dstSet = m_descriptorSets[j];
			lv_writes[i + 8].pBufferInfo = nullptr;
			lv_writes[i + 8].pImageInfo = &lv_imageInfos[9 * j + 6];
			lv_writes[i + 8].pNext = nullptr;
			lv_writes[i + 8].pTexelBufferView = nullptr;
			lv_writes[i + 8].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 9].descriptorCount = 1;
			lv_writes[i + 9].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 9].dstArrayElement = 0;
			lv_writes[i + 9].dstBinding = 9;
			lv_writes[i + 9].dstSet = m_descriptorSets[j];
			lv_writes[i + 9].pBufferInfo = nullptr;
			lv_writes[i + 9].pImageInfo = &lv_imageInfos[9 * j + 7];
			lv_writes[i + 9].pNext = nullptr;
			lv_writes[i + 9].pTexelBufferView = nullptr;
			lv_writes[i + 9].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;


			lv_writes[i + 10].descriptorCount = 1;
			lv_writes[i + 10].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			lv_writes[i + 10].dstArrayElement = 0;
			lv_writes[i + 10].dstBinding = 10;
			lv_writes[i + 10].dstSet = m_descriptorSets[j];
			lv_writes[i + 10].pBufferInfo = &lv_bufferInfos[2];
			lv_writes[i + 10].pImageInfo = nullptr;
			lv_writes[i + 10].pNext = nullptr;
			lv_writes[i + 10].pTexelBufferView = nullptr;
			lv_writes[i + 10].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 11].descriptorCount = 1;
			lv_writes[i + 11].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 11].dstArrayElement = 0;
			lv_writes[i + 11].dstBinding = 11;
			lv_writes[i + 11].dstSet = m_descriptorSets[j];
			lv_writes[i + 11].pBufferInfo = nullptr;
			lv_writes[i + 11].pImageInfo = &lv_imageInfos[9 * j + 8];
			lv_writes[i + 11].pNext = nullptr;
			lv_writes[i + 11].pTexelBufferView = nullptr;
			lv_writes[i + 11].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		}

		vkUpdateDescriptorSets(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device
			, lv_writes.size(), lv_writes.data(), 0, nullptr);
	}


	void DeferredLightningRenderer::UpdateBuffers
	(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure)
	{
		UniformBuffer lv_cameraUniform{};
		lv_cameraUniform.m_cameraPos = glm::vec4{ l_cameraStructure.m_cameraPos, 1.f };
		lv_cameraUniform.m_viewMatrix = l_cameraStructure.m_viewMatrix;
		lv_cameraUniform.m_inMtx = l_cameraStructure.m_projectionMatrix * l_cameraStructure.m_viewMatrix;
		lv_cameraUniform.m_time = glm::vec4{ (float)glfwGetTime() };

		auto& lv_uniformBufferGpu = m_vulkanRenderContext.GetResourceManager().RetrieveGpuBuffer(m_uniformBufferGpuHandle);
		memcpy(lv_uniformBufferGpu.ptr, &lv_cameraUniform, lv_uniformBufferGpu.size);

	}

	void DeferredLightningRenderer::FillCommandBuffer
	(VkCommandBuffer l_cmdBuffer, uint32_t l_currentSwapchainIndex)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();

		auto& lv_gbufferTangentGpu = lv_vkResManager.RetrieveGpuTexture("GBufferTangent", l_currentSwapchainIndex);
		auto& lv_gbufferPosGpu = lv_vkResManager.RetrieveGpuTexture("GBufferPosition", l_currentSwapchainIndex);
		auto& lv_gbufferNormalGpu = lv_vkResManager.RetrieveGpuTexture("GBufferNormal", l_currentSwapchainIndex);
		auto& lv_gbufferAlbedoSpecGpu = lv_vkResManager.RetrieveGpuTexture("GBufferAlbedoSpec", l_currentSwapchainIndex);
		auto& lv_gbufferNormalVertexGpu = lv_vkResManager.RetrieveGpuTexture("GBufferNormalVertex", l_currentSwapchainIndex);
		auto lv_framebuffer = lv_vkResManager.RetrieveGpuFramebuffer(m_framebufferHandles[l_currentSwapchainIndex]);
		/*auto& lv_vertexBuffer = lv_vkResManager.RetrieveGpuBuffer(m_vertexBufferGpuHandle);
		auto& lv_indexBuffer = lv_vkResManager.RetrieveGpuBuffer(m_indicesBufferGpuHandle);*/
		auto& lv_depth = lv_vkResManager.RetrieveGpuTexture("Depth", l_currentSwapchainIndex);
		auto& lv_metallicGpu = lv_vkResManager.RetrieveGpuTexture("GBufferMetallic", l_currentSwapchainIndex);
		//auto& lv_depthMapLight = lv_vkResManager.RetrieveGpuTexture(m_depthMapLightGpuHandle);

		


		//transitionImageLayoutCmd(l_cmdBuffer, lv_gbufferTangentGpu.image.image
		//	, lv_gbufferTangentGpu.format, lv_gbufferTangentGpu.Layout
		//	, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		//transitionImageLayoutCmd(l_cmdBuffer, lv_gbufferPosGpu.image.image
		//						,lv_gbufferPosGpu.format, lv_gbufferPosGpu.Layout
		//						,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		//transitionImageLayoutCmd(l_cmdBuffer, lv_gbufferNormalGpu.image.image
		//						, lv_gbufferNormalGpu.format, lv_gbufferNormalGpu.Layout
		//						, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		//transitionImageLayoutCmd(l_cmdBuffer, lv_gbufferAlbedoSpecGpu.image.image
		//						, lv_gbufferAlbedoSpecGpu.format, lv_gbufferAlbedoSpecGpu.Layout
		//						, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		//transitionImageLayoutCmd(l_cmdBuffer, lv_gbufferNormalVertexGpu.image.image
		//	, lv_gbufferNormalVertexGpu.format, lv_gbufferNormalVertexGpu.Layout
		//	, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		//transitionImageLayoutCmd(l_cmdBuffer, lv_depth.image.image
		//	, lv_depth.format, lv_depth.Layout
		//	, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		///*transitionImageLayoutCmd(l_cmdBuffer, lv_occlusionGpu.image.image
		//	, lv_occlusionGpu.format, lv_occlusionGpu.Layout
		//	, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);*/
		//transitionImageLayoutCmd(l_cmdBuffer, lv_metallicGpu.image.image
		//	, lv_metallicGpu.format, lv_metallicGpu.Layout
		//	, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		transitionImageLayoutCmd(l_cmdBuffer, m_colorOutputTextures[l_currentSwapchainIndex]->image.image
			, m_colorOutputTextures[l_currentSwapchainIndex]->format, m_colorOutputTextures[l_currentSwapchainIndex]->Layout
			, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		/*transitionImageLayoutCmd(l_cmdBuffer, lv_depthMapLight.image.image
			, lv_depthMapLight.format, lv_depthMapLight.Layout
			, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6);*/
		/*transitionImageLayoutCmd(l_cmdBuffer, m_colorOutputTextures[l_currentSwapchainIndex]->image.image
			, m_colorOutputTextures[l_currentSwapchainIndex]->format, m_colorOutputTextures[l_currentSwapchainIndex]->Layout
			, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);*/



		//lv_gbufferTangentGpu.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		//lv_gbufferPosGpu.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		//lv_gbufferNormalGpu.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		//lv_gbufferAlbedoSpecGpu.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		//lv_gbufferNormalVertexGpu.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		//lv_depth.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		////lv_occlusionGpu.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		//lv_metallicGpu.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		m_colorOutputTextures[l_currentSwapchainIndex]->Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		//lv_depthMapLight.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		//m_colorOutputTextures[l_currentSwapchainIndex]->Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		/*VkDeviceSize lv_offset{ 0 };
		vkCmdBindVertexBuffers(l_cmdBuffer, 0, 1, &lv_vertexBuffer.buffer, &lv_offset);
		vkCmdBindIndexBuffer(l_cmdBuffer, lv_indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);*/

		BeginRenderPass(m_renderPass, lv_framebuffer, l_cmdBuffer, l_currentSwapchainIndex, 1);
		vkCmdDraw(l_cmdBuffer, 6, 1, 0, 0);
		vkCmdEndRenderPass(l_cmdBuffer);
		m_colorOutputTextures[l_currentSwapchainIndex]->Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		/*transitionImageLayoutCmd(l_cmdBuffer, lv_depthMapLight.image.image
			, lv_depthMapLight.format, lv_depthMapLight.Layout
			, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 6);
		lv_depthMapLight.Layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;*/

		

		//lv_depthMapLight.Layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


	}

	void DeferredLightningRenderer::InitializePositionData
	(std::array<glm::vec4, m_totalNumLights>& l_positionData)
	{
		for (uint32_t i = 0; i < 8; ++i) {
			l_positionData[i] = glm::vec4{ -45.f + (float)20*i, 0.5f, 4.f, (float)i };
		}

		for (uint32_t i = 0; i < 8; ++i) {
			l_positionData[i + 8] = glm::vec4{ -45.f + (float)20*i, 0.5f, -6.f, (float)i };
		}
	}

	void DeferredLightningRenderer::InitializeLightBuffer
	(std::array<Light, m_totalNumLights>& l_lightBuffer
	, const std::array<glm::vec4, m_totalNumLights>& l_positionData)
	{
		for (uint32_t i = 0; i < m_totalNumLights; ++i) {
			l_lightBuffer[i].m_position = l_positionData[i];
			l_lightBuffer[i].m_position.w = 1.f;
		}
	}


	void DeferredLightningRenderer::InitializeVertexBuffer
	(const std::array<glm::vec4, m_totalNumLights>& l_positionData
	,std::array<glm::vec4, 8*m_totalNumLights>& l_vertexBuffer
	,const std::array<glm::vec4, 8>& l_unitCube)
	{
		for (uint32_t i = 0, d = 0; i < l_vertexBuffer.size(); i+=8, ++d) {
			for (uint32_t j = 0; j < 8; ++j) {
				l_vertexBuffer[i + j] = l_positionData[d] + l_unitCube[j];
			}
		}
	}
}