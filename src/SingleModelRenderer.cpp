



#include "SingleModelRenderer.hpp"
#include "CameraStructure.hpp"


namespace RenderCore
{

	SingleModelRenderer::SingleModelRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator)
		:Renderbase(l_vkContextCreator)
	{

	}


	void SingleModelRenderer::Init(const std::vector<glm::vec3>& l_vertices
		, const std::vector<uint32_t> l_indices
		, const char* l_vtxShader, const char* l_fragShader
		, const char* l_spv)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_totalNumSwapchain = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();
		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();

		m_uniformBufferGpuHandle = lv_vkResManager.CreateBufferWithHandle(sizeof(UniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "SingleModelRendererUniformBuffer");
		m_lightUniformBufferGpuHandle = lv_vkResManager.CreateBufferWithHandle(sizeof(LightUniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "SingleModelRendererLightUniformBuffer");

		m_depthTextures.resize(lv_totalNumSwapchain);
		m_colorOutputTextures.resize(lv_totalNumSwapchain);
		for (size_t i = 0; i < lv_totalNumSwapchain; ++i) {
			m_colorOutputTextures[i] = &lv_vkResManager.RetrieveGpuTexture("DeferredLightningColorTexture", i);
			m_depthTextures[i] = &lv_vkResManager.RetrieveGpuTexture("Depth", i);
		}


		m_vertexBufferGpuHandle = lv_vkResManager.CreateBufferWithHandle(l_vertices.size()*sizeof(glm::vec3),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			"SingleModelRendererVertexBuffer");
		m_indexBufferGpuHandle = lv_vkResManager.CreateBufferWithHandle(sizeof(uint32_t)*l_indices.size(),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			"SingleModelRendererIndexBuffer");


		auto& lv_vertexBufferGpu = lv_vkResManager.RetrieveGpuBuffer(m_vertexBufferGpuHandle);
		auto& lv_indexBufferGpu = lv_vkResManager.RetrieveGpuBuffer(m_indexBufferGpuHandle);

		memcpy(lv_vertexBufferGpu.ptr, l_vertices.data(), sizeof(glm::vec3) * l_vertices.size());
		memcpy(lv_indexBufferGpu.ptr, l_indices.data(), sizeof(uint32_t) * l_indices.size());

		m_indexCount = (uint32_t)l_indices.size();
		GeneratePipelineFromSpirvBinaries(l_spv);
		SetRenderPassAndFrameBuffer("PointLightCube");
		SetNodeToAppropriateRenderpass("PointLightCube", this);
		UpdateDescriptorSets();
		lv_frameGraph.IncrementNumNodesPerCmdBuffer(2);

		auto* lv_node = lv_frameGraph.RetrieveNode("PointLightCube");
		
		VkVertexInputBindingDescription lv_vtxBindingDesc1{};
		lv_vtxBindingDesc1.binding = 0;
		lv_vtxBindingDesc1.stride = sizeof(glm::vec3);
		lv_vtxBindingDesc1.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription lv_vtxAttribDesc10{};
		lv_vtxAttribDesc10.binding = 0;
		lv_vtxAttribDesc10.format = VK_FORMAT_R32G32B32_SFLOAT;
		lv_vtxAttribDesc10.location = 0;
		lv_vtxAttribDesc10.offset = 0;

		VulkanResourceManager::PipelineInfo lv_pipelineInfo;
		lv_pipelineInfo.m_dynamicScissorState = false;
		lv_pipelineInfo.m_height = 1024;
		lv_pipelineInfo.m_width = 1024;
		lv_pipelineInfo.m_useBlending = false;
		lv_pipelineInfo.m_useDepth = true;
		lv_pipelineInfo.m_enableWireframe = false;
		lv_pipelineInfo.m_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		lv_pipelineInfo.m_totalNumColorAttach = ((uint32_t)lv_node->m_outputResourcesHandles.size()) - 1;

		lv_pipelineInfo.m_vertexInputBindingDescription.push_back(lv_vtxBindingDesc1);
		lv_pipelineInfo.m_vertexInputAttribDescription.push_back(lv_vtxAttribDesc10);

		m_graphicsPipeline = lv_vkResManager.CreateGraphicsPipeline(m_renderPass, m_pipelineLayout,
			{ l_vtxShader, l_fragShader }, "SingleModelRenderer",
			lv_pipelineInfo);

	}



	void SingleModelRenderer::FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
		uint32_t l_currentSwapchainIndex)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();

		auto lv_framebuffer = lv_vkResManager.RetrieveGpuFramebuffer(m_framebufferHandles[l_currentSwapchainIndex]);

		transitionImageLayoutCmd(l_cmdBuffer, m_colorOutputTextures[l_currentSwapchainIndex]->image.image
			, m_colorOutputTextures[l_currentSwapchainIndex]->format
			,m_colorOutputTextures[l_currentSwapchainIndex]->Layout
			, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		transitionImageLayoutCmd(l_cmdBuffer, m_depthTextures[l_currentSwapchainIndex]->image.image
			, m_depthTextures[l_currentSwapchainIndex]->format
			, m_depthTextures[l_currentSwapchainIndex]->Layout
			, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		m_depthTextures[l_currentSwapchainIndex]->Layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		m_colorOutputTextures[l_currentSwapchainIndex]->Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkDeviceSize lv_offset = 0;
		auto& lv_vertexBufferGpu = lv_vkResManager.RetrieveGpuBuffer(m_vertexBufferGpuHandle);
		auto& lv_indexBufferGpu = lv_vkResManager.RetrieveGpuBuffer(m_indexBufferGpuHandle);

		vkCmdBindIndexBuffer(l_cmdBuffer, lv_indexBufferGpu.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindVertexBuffers(l_cmdBuffer, 0, 1, &lv_vertexBufferGpu.buffer, &lv_offset);

		BeginRenderPass(m_renderPass, lv_framebuffer, l_cmdBuffer, l_currentSwapchainIndex, 1
			, 1024
			, 1024);
		vkCmdDrawIndexed(l_cmdBuffer, m_indexCount, 1, 0,0, 0);
		vkCmdEndRenderPass(l_cmdBuffer);

		m_colorOutputTextures[l_currentSwapchainIndex]->Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		m_depthTextures[l_currentSwapchainIndex]->Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;




	}


	void SingleModelRenderer::SetLightIntensity(const float l_lightIntensity)
	{
		m_lightIntensity = l_lightIntensity;
	}


	void SingleModelRenderer::UpdateBuffers(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure)
	{
		UniformBuffer lv_uniformCpu{};
		lv_uniformCpu.m_cameraPos = glm::vec4{ l_cameraStructure.m_cameraPos, 1.f };
		lv_uniformCpu.m_projectionMatrix = l_cameraStructure.m_projectionMatrix;
		lv_uniformCpu.m_viewMatrix = l_cameraStructure.m_viewMatrix;;

		LightUniformBuffer lv_lightUniform{};
		lv_lightUniform.m_lightIntensity = m_lightIntensity;

		auto& lv_uniformBufferGpu = m_vulkanRenderContext.GetResourceManager().RetrieveGpuBuffer(m_uniformBufferGpuHandle);
		auto& lv_lightUniformBufferGpu = m_vulkanRenderContext.GetResourceManager().RetrieveGpuBuffer(m_lightUniformBufferGpuHandle);


		memcpy(lv_uniformBufferGpu.ptr, &lv_uniformCpu, sizeof(UniformBuffer));
		memcpy(lv_lightUniformBufferGpu.ptr, &lv_lightUniform, sizeof(LightUniformBuffer));



	}

	void SingleModelRenderer::UpdateDescriptorSets()
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();


		std::array<VkDescriptorBufferInfo, 2> lv_bufferInfo{};

		lv_bufferInfo[0].buffer = lv_vkResManager.RetrieveGpuBuffer(m_uniformBufferGpuHandle).buffer;
		lv_bufferInfo[0].offset = 0;
		lv_bufferInfo[0].range = VK_WHOLE_SIZE;

		lv_bufferInfo[1].buffer = lv_vkResManager.RetrieveGpuBuffer(m_lightUniformBufferGpuHandle).buffer;
		lv_bufferInfo[1].offset = 0;
		lv_bufferInfo[1].range = VK_WHOLE_SIZE;

		std::vector<VkWriteDescriptorSet> lv_writes{};
		lv_writes.resize(2*lv_totalNumSwapchains);

		for (size_t i = 0, j = 0; i < lv_writes.size(); i+=2, ++j) {
			lv_writes[i].descriptorCount = 1;
			lv_writes[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			lv_writes[i].dstArrayElement = 0;
			lv_writes[i].dstBinding = 0;
			lv_writes[i].dstSet = m_descriptorSets[j];
			lv_writes[i].pBufferInfo = &lv_bufferInfo[0];
			lv_writes[i].pImageInfo = nullptr;
			lv_writes[i].pNext = nullptr;
			lv_writes[i].pTexelBufferView = nullptr;
			lv_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;


			lv_writes[i+1].descriptorCount = 1;
			lv_writes[i+1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			lv_writes[i+1].dstArrayElement = 0;
			lv_writes[i+1].dstBinding = 1;
			lv_writes[i+1].dstSet = m_descriptorSets[j];
			lv_writes[i+1].pBufferInfo = &lv_bufferInfo[1];
			lv_writes[i+1].pImageInfo = nullptr;
			lv_writes[i+1].pNext = nullptr;
			lv_writes[i+1].pTexelBufferView = nullptr;
			lv_writes[i+1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		}


		vkUpdateDescriptorSets(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device, lv_writes.size(), lv_writes.data(), 0, nullptr );

	}

}