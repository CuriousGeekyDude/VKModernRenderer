


#include "DeferredLightningRenderer.hpp"
#include <format>
#include "CameraStructure.hpp"

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
		/*m_vertexBufferGpuHandle = lv_vkResManager.CreateBufferWithHandle
		(sizeof(glm::vec4) * 8*m_totalNumLights, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "VertexBufferDeferredRenderpass");
		m_indicesBufferGpuHandle = lv_vkResManager.CreateBufferWithHandle
		(sizeof(uint16_t)*36, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		,"IndexBufferDeferredRenderpass");*/
		m_uniformBufferGpuHandle = lv_vkResManager.CreateBufferWithHandle
		(sizeof(UniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
			, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			, "UniformBufferDeferredRenderpass");

		/*lv_vkResManager.CopyDataToLocalBuffer(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_mainQueue2,
			m_vulkanRenderContext.GetContextCreator().m_vkDev.m_mainCommandBuffer1[0], m_vertexBufferGpuHandle
		, lv_vertexBuffer.data());
		lv_vkResManager.CopyDataToLocalBuffer(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_mainQueue2,
			m_vulkanRenderContext.GetContextCreator().m_vkDev.m_mainCommandBuffer1[0], m_indicesBufferGpuHandle
			, lv_indexBuffer.data());*/
		

		auto& lv_lightGpu = lv_vkResManager.RetrieveGpuBuffer(m_lightBufferGpuHandle);
		memcpy(lv_lightGpu.ptr, lv_lightData.data(), lv_lightData.size()*sizeof(Light));

		GeneratePipelineFromSpirvBinaries(l_spvPath);
		SetRenderPassAndFrameBuffer("DeferredLightning");
		SetNodeToAppropriateRenderpass("DeferredLightning", this);
		UpdateDescriptorSets();

		

		/*VkVertexInputBindingDescription lv_vtxBindingDesc{};
		lv_vtxBindingDesc.binding = 0;
		lv_vtxBindingDesc.stride = sizeof(glm::vec4);
		lv_vtxBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription lv_firstAttribDesc{};
		lv_firstAttribDesc.binding = 0;
		lv_firstAttribDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
		lv_firstAttribDesc.location = 0;
		lv_firstAttribDesc.offset = 0;

		VkVertexInputAttributeDescription lv_secondAttribDesc{};
		lv_secondAttribDesc.binding = 0;
		lv_secondAttribDesc.location = 1;
		lv_secondAttribDesc.offset = sizeof(glm::vec3);
		lv_secondAttribDesc.format = VK_FORMAT_R32_SFLOAT;*/

		auto* lv_node = lv_frameGraph.RetrieveNode("DeferredLightning");
		VulkanResourceManager::PipelineInfo lv_pipeInfo{};
		lv_pipeInfo.m_dynamicScissorState = false;
		lv_pipeInfo.m_enableWireframe = false;
		lv_pipeInfo.m_height = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferHeight;
		lv_pipeInfo.m_width = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferWidth;
		lv_pipeInfo.m_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		lv_pipeInfo.m_useBlending = false;
		lv_pipeInfo.m_useDepth = true;
		lv_pipeInfo.m_totalNumColorAttach = lv_node->m_outputResourcesHandles.size() - 1;
		/*lv_pipeInfo.m_vertexInputBindingDescription.push_back(lv_vtxBindingDesc);
		lv_pipeInfo.m_vertexInputAttribDescription.push_back(lv_firstAttribDesc);
		lv_pipeInfo.m_vertexInputAttribDescription.push_back(lv_secondAttribDesc);*/

		m_graphicsPipeline = lv_vkResManager.CreateGraphicsPipeline(m_renderPass, m_pipelineLayout
			, {l_vtxShader, l_fragShader},"GraphicsPipelineDeferredLightning", lv_pipeInfo);
	}


	void DeferredLightningRenderer::UpdateDescriptorSets()
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();

		std::array<VkDescriptorBufferInfo, 2> lv_bufferInfos{};
		std::vector<VkDescriptorImageInfo> lv_imageInfos{};
		lv_imageInfos.resize(lv_totalNumSwapchains * 3);


		auto& lv_lightBufferGpu = lv_vkResManager.RetrieveGpuBuffer(m_lightBufferGpuHandle);
		auto& lv_uniformBufferGpu = lv_vkResManager.RetrieveGpuBuffer(m_uniformBufferGpuHandle);

		lv_bufferInfos[0].buffer = lv_uniformBufferGpu.buffer;
		lv_bufferInfos[0].offset = 0;
		lv_bufferInfos[0].range = VK_WHOLE_SIZE;

		lv_bufferInfos[1].buffer = lv_lightBufferGpu.buffer;
		lv_bufferInfos[1].offset = 0;
		lv_bufferInfos[1].range = VK_WHOLE_SIZE;

		for (size_t i = 0, j = 0; i < lv_imageInfos.size(); i+=3, ++j) {
			
			auto& lv_gbufferPosGpu = lv_vkResManager.RetrieveGpuTexture("GBufferPosition", j);
			auto& lv_gbufferNormalGpu = lv_vkResManager.RetrieveGpuTexture("GBufferNormal", j);
			auto& lv_gbufferAlbedoSpec = lv_vkResManager.RetrieveGpuTexture("GBufferAlbedoSpec", j);

			lv_imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfos[i].imageView = lv_gbufferPosGpu.image.imageView;
			lv_imageInfos[i].sampler = lv_gbufferPosGpu.sampler;

			lv_imageInfos[i + 1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfos[i + 1].imageView = lv_gbufferNormalGpu.image.imageView;
			lv_imageInfos[i + 1].sampler = lv_gbufferNormalGpu.sampler;

			lv_imageInfos[i + 2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfos[i + 2].imageView = lv_gbufferAlbedoSpec.image.imageView;
			lv_imageInfos[i + 2].sampler = lv_gbufferAlbedoSpec.sampler;

		}

		std::vector<VkWriteDescriptorSet> lv_writes;
		lv_writes.resize(lv_totalNumSwapchains*lv_bufferInfos.size() + lv_imageInfos.size());


		for (size_t i = 0, j = 0; i < lv_writes.size(); i += 5, ++j) {

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
			lv_writes[i+2].pImageInfo = &lv_imageInfos[3*j];
			lv_writes[i+2].pNext = nullptr;
			lv_writes[i+2].pTexelBufferView = nullptr;
			lv_writes[i+2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 3].descriptorCount = 1;
			lv_writes[i + 3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 3].dstArrayElement = 0;
			lv_writes[i + 3].dstBinding = 3;
			lv_writes[i + 3].dstSet = m_descriptorSets[j];
			lv_writes[i + 3].pBufferInfo = nullptr;
			lv_writes[i + 3].pImageInfo = &lv_imageInfos[3 * j + 1];
			lv_writes[i + 3].pNext = nullptr;
			lv_writes[i + 3].pTexelBufferView = nullptr;
			lv_writes[i + 3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 4].descriptorCount = 1;
			lv_writes[i + 4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[i + 4].dstArrayElement = 0;
			lv_writes[i + 4].dstBinding = 4;
			lv_writes[i + 4].dstSet = m_descriptorSets[j];
			lv_writes[i + 4].pBufferInfo = nullptr;
			lv_writes[i + 4].pImageInfo = &lv_imageInfos[3 * j + 2];
			lv_writes[i + 4].pNext = nullptr;
			lv_writes[i + 4].pTexelBufferView = nullptr;
			lv_writes[i + 4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
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


		auto& lv_uniformBufferGpu = m_vulkanRenderContext.GetResourceManager().RetrieveGpuBuffer(m_uniformBufferGpuHandle);
		memcpy(lv_uniformBufferGpu.ptr, &lv_cameraUniform, lv_uniformBufferGpu.size);

	}

	void DeferredLightningRenderer::FillCommandBuffer
	(VkCommandBuffer l_cmdBuffer, uint32_t l_currentSwapchainIndex)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();

		auto& lv_gbufferPosGpu = lv_vkResManager.RetrieveGpuTexture("GBufferPosition", l_currentSwapchainIndex);
		auto& lv_gbufferNormalGpu = lv_vkResManager.RetrieveGpuTexture("GBufferNormal", l_currentSwapchainIndex);
		auto& lv_gbufferAlbedoSpecGpu = lv_vkResManager.RetrieveGpuTexture("GBufferAlbedoSpec", l_currentSwapchainIndex);
		auto lv_framebuffer = lv_vkResManager.RetrieveGpuFramebuffer(m_framebufferHandles[l_currentSwapchainIndex]);
		/*auto& lv_vertexBuffer = lv_vkResManager.RetrieveGpuBuffer(m_vertexBufferGpuHandle);
		auto& lv_indexBuffer = lv_vkResManager.RetrieveGpuBuffer(m_indicesBufferGpuHandle);*/
		auto& lv_depth = lv_vkResManager.RetrieveGpuTexture("Depth", l_currentSwapchainIndex);

		transitionImageLayoutCmd(l_cmdBuffer, lv_gbufferPosGpu.image.image
								,lv_gbufferPosGpu.format, lv_gbufferPosGpu.Layout
								,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		transitionImageLayoutCmd(l_cmdBuffer, lv_gbufferNormalGpu.image.image
								, lv_gbufferNormalGpu.format, lv_gbufferNormalGpu.Layout
								, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		transitionImageLayoutCmd(l_cmdBuffer, lv_gbufferAlbedoSpecGpu.image.image
								, lv_gbufferAlbedoSpecGpu.format, lv_gbufferAlbedoSpecGpu.Layout
								, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		transitionImageLayoutCmd(l_cmdBuffer, lv_depth.image.image
			, lv_depth.format, lv_depth.Layout
			, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		lv_gbufferPosGpu.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		lv_gbufferNormalGpu.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		lv_gbufferAlbedoSpecGpu.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		lv_depth.Layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		/*VkDeviceSize lv_offset{ 0 };
		vkCmdBindVertexBuffers(l_cmdBuffer, 0, 1, &lv_vertexBuffer.buffer, &lv_offset);
		vkCmdBindIndexBuffer(l_cmdBuffer, lv_indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);*/

		BeginRenderPass(m_renderPass, lv_framebuffer, l_cmdBuffer, l_currentSwapchainIndex, 1);
		vkCmdDraw(l_cmdBuffer, 6, 1, 0, 0);
		vkCmdEndRenderPass(l_cmdBuffer);


	}

	void DeferredLightningRenderer::InitializePositionData
	(std::array<glm::vec4, m_totalNumLights>& l_positionData)
	{
		for (uint32_t i = 0; i < 16; ++i) {
			l_positionData[i] = glm::vec4{ -12.f + (float)i, 1.f, 1.5f, (float)i };
		}

		for (uint32_t i = 16; i < m_totalNumLights; ++i) {
			l_positionData[i] = glm::vec4{ -12.f + (float)i, 1.f, -2.f, (float)i };
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