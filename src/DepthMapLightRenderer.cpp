



#include "DepthMapLightRenderer.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "IndirectRenderer.hpp"
#include <GLFW/glfw3.h>

namespace RenderCore
{
	

	DepthMapLightRenderer::DepthMapLightRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator
		, const char* l_vtxShader, const char* l_fragShader
		, const char* l_spvFile, const char* l_rendererName
		, const glm::vec3& l_lightPos, const glm::vec3& l_lookAtVector
		, const glm::vec3& l_up, const int l_cubemapFace)
		:Renderbase(l_vkContextCreator)
		,m_rendererName{l_rendererName}
		,m_cubemapFace(l_cubemapFace)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();
		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();
		auto* lv_indirectRenderer = (IndirectRenderer*)lv_frameGraph.RetrieveNode("IndirectGbuffer")->m_renderer;


		float lv_aspect = (float)m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferWidth / (float)m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferHeight;

		m_uniformBufferCpu.m_pos = glm::vec4{ l_lightPos, 1.f };
		m_uniformBufferCpu.m_viewMatrix = glm::lookAt(l_lightPos, l_lookAtVector, l_up);
		m_uniformBufferCpu.m_projMatrix = glm::perspective(glm::radians(90.f),lv_aspect, 0.1f, 100.f);

		m_uniformBufferGpuHandle = lv_vkResManager.CreateBufferWithHandle(sizeof(UniformBufferLight), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
																		 , VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
																		 , "UniformBufferLightMatricesDepthMap");


		auto lv_depthMapMeta = lv_vkResManager.RetrieveGpuResourceMetaData("DepthMapPointLight");

		m_depthMapGpuTextures.push_back(&lv_vkResManager.RetrieveGpuTexture(lv_depthMapMeta.m_resourceHandle));
		m_instanceBuffersGpu.resize(lv_totalNumSwapchains);
		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
			m_instanceBuffersGpu[i] = &lv_vkResManager.RetrieveGpuBuffer("Instance-Buffer-Indirect", i);
		}



		auto& lv_outputInstanceData = lv_indirectRenderer->GetInstanceData();
		auto& lv_meshes = lv_indirectRenderer->GetMeshData();

		auto lv_indirectBufferMeta = lv_vkResManager.RetrieveGpuResourceMetaData("indirectBufferDepthMapLight");

		bool lv_indirectBufferCreatedBefore = (lv_indirectBufferMeta.m_resourceHandle != std::numeric_limits<uint32_t>::max());

		m_indirectBufferGpuHandle = true == lv_indirectBufferCreatedBefore ? lv_indirectBufferMeta.m_resourceHandle
									: lv_vkResManager.CreateBufferWithHandle(sizeof(VkDrawIndirectCommand) * lv_outputInstanceData.size()
										, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
										, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
										, "indirectBufferDepthMapLight");

		if (false == lv_indirectBufferCreatedBefore) {
			auto& lv_indirectBuffer = lv_vkResManager.RetrieveGpuBuffer(m_indirectBufferGpuHandle);
			VkDrawIndirectCommand* lv_drawStructure = (VkDrawIndirectCommand*)lv_indirectBuffer.ptr;
			for (uint32_t i = 0; i < lv_outputInstanceData.size(); ++i) {
				auto j = lv_outputInstanceData[i].m_meshIndex;
				lv_drawStructure[i].vertexCount = lv_meshes[j].CalculateLODNumberOfIndices(lv_outputInstanceData[i].m_lod);
				lv_drawStructure[i].firstInstance = i;
				lv_drawStructure[i].firstVertex = 0U;
				lv_drawStructure[i].instanceCount = 1;

			}
		}



		auto& lv_uniformBufferGpu = lv_vkResManager.RetrieveGpuBuffer(m_uniformBufferGpuHandle);
		memcpy(lv_uniformBufferGpu.ptr, &m_uniformBufferCpu, sizeof(UniformBufferLight));


		auto lv_indicesVerticesMetaData = lv_vkResManager.RetrieveGpuResourceMetaData(" Vertex-Buffer-Indirect ");

		assert(std::numeric_limits<float>::max() != lv_indicesVerticesMetaData.m_resourceHandle);
		m_indicesVerticesGpuBufferHandle = lv_indicesVerticesMetaData.m_resourceHandle;

		GeneratePipelineFromSpirvBinaries(l_spvFile);
		SetRenderPassAndFrameBuffer(l_rendererName);
		SetNodeToAppropriateRenderpass(l_rendererName, this);
		UpdateDescriptorSets();

		auto* lv_node = lv_frameGraph.RetrieveNode(l_rendererName);
		lv_frameGraph.IncrementNumNodesPerCmdBuffer(1);

		VulkanResourceManager::PipelineInfo lv_pipeInfo{};
		lv_pipeInfo.m_dynamicScissorState = false;
		lv_pipeInfo.m_enableWireframe = false;
		lv_pipeInfo.m_height = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferHeight;
		lv_pipeInfo.m_width = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferWidth;
		lv_pipeInfo.m_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		lv_pipeInfo.m_useBlending = false;
		lv_pipeInfo.m_useDepth = true;
		lv_pipeInfo.m_totalNumColorAttach = lv_node->m_outputResourcesHandles.size()-1;

		std::string lv_pipelineName{ "GraphicsPipeline" };
		lv_pipelineName += l_rendererName;
		m_graphicsPipeline = lv_vkResManager.CreateGraphicsPipeline(m_renderPass, m_pipelineLayout
			, { l_vtxShader, l_fragShader }, lv_pipelineName.c_str(), lv_pipeInfo);
	}

	void DepthMapLightRenderer::FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
		uint32_t l_currentSwapchainIndex)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();

		auto* lv_currentNode = lv_frameGraph.RetrieveNode(m_rendererName);


		auto* lv_currDepthMapTex = m_depthMapGpuTextures[0];


		/*if (VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL == lv_currDepthMapTex->Layout) {
			transitionImageLayoutCmd(l_cmdBuffer, lv_currDepthMapTex->image.image, lv_currDepthMapTex->format
				, lv_currDepthMapTex->Layout, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,6);
			lv_currDepthMapTex->Layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}*/

		VkImageLayout lv_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		
		switch (m_cubemapFace) {
		case 0:
			lv_layout = lv_currDepthMapTex->l_cubemapFace0Layout;
			lv_currDepthMapTex->l_cubemapFace0Layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			break;
		case 1:
			lv_layout = lv_currDepthMapTex->l_cubemapFace1Layout;
			lv_currDepthMapTex->l_cubemapFace1Layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			break;
		case 2:
			lv_layout = lv_currDepthMapTex->l_cubemapFace2Layout;
			lv_currDepthMapTex->l_cubemapFace2Layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			break;
		case 3:
			lv_layout = lv_currDepthMapTex->l_cubemapFace3Layout;
			lv_currDepthMapTex->l_cubemapFace3Layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			break;
		case 4:
			lv_layout = lv_currDepthMapTex->l_cubemapFace4Layout;
			lv_currDepthMapTex->l_cubemapFace4Layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			break;
		case 5:
			lv_layout = lv_currDepthMapTex->l_cubemapFace5Layout;
			lv_currDepthMapTex->l_cubemapFace5Layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			break;
		
		}
		transitionImageLayoutCmd(l_cmdBuffer, lv_currDepthMapTex->image.image, lv_currDepthMapTex->format
			, lv_layout, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, 1, m_cubemapFace);

		/*bool lv_allLayersTransitioned = (lv_currDepthMapTex->l_cubemapFace0Layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) &&
			(lv_currDepthMapTex->l_cubemapFace1Layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) &&
			(lv_currDepthMapTex->l_cubemapFace2Layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) &&
			(lv_currDepthMapTex->l_cubemapFace3Layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) &&
			(lv_currDepthMapTex->l_cubemapFace4Layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) &&
			(lv_currDepthMapTex->l_cubemapFace5Layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);*/



		auto& lv_indirectBuffer = lv_vkResManager.RetrieveGpuBuffer(m_indirectBufferGpuHandle);
		auto* lv_indirectRenderer = (IndirectRenderer*)lv_frameGraph.RetrieveNode("IndirectGbuffer")->m_renderer;
		auto lv_totalNumInstances = lv_indirectRenderer->GetInstanceData().size();


		

		auto lv_framebuffer = lv_vkResManager.RetrieveGpuFramebuffer(m_framebufferHandles[l_currentSwapchainIndex]);

		BeginRenderPass(m_renderPass, lv_framebuffer, l_cmdBuffer, l_currentSwapchainIndex, 1);
		vkCmdDrawIndirect(l_cmdBuffer, lv_indirectBuffer.buffer, 0, lv_totalNumInstances,
			sizeof(VkDrawIndirectCommand));
		vkCmdEndRenderPass(l_cmdBuffer);

		

		switch (m_cubemapFace) {
		case 0:
			lv_currDepthMapTex->l_cubemapFace0Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			break;
		case 1:
			lv_currDepthMapTex->l_cubemapFace1Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			break;
		case 2:
			lv_currDepthMapTex->l_cubemapFace2Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			break;
		case 3:
			lv_currDepthMapTex->l_cubemapFace3Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			break;
		case 4:
			lv_currDepthMapTex->l_cubemapFace4Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			break;
		case 5:
			lv_currDepthMapTex->l_cubemapFace5Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			break;

		}

		lv_currentNode->m_enabled = false;

	}

	void DepthMapLightRenderer::UpdateBuffers(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure)
	{
		/*auto& lv_uniformBufferGpu = m_vulkanRenderContext.GetResourceManager().RetrieveGpuBuffer(m_uniformBufferGpuHandle);
		float lv_time = glfwGetTime();

		m_uniformBufferCpu.m_pos.x = 20.f * std::sin(std::log10(lv_time));


		glm::vec3 lv_lightPos{ m_uniformBufferCpu.m_pos.x, m_uniformBufferCpu.m_pos.y, m_uniformBufferCpu.m_pos.z};
		switch (m_cubemapFace) {
		case 0:
			m_uniformBufferCpu.m_viewMatrix = glm::lookAt(lv_lightPos, lv_lightPos+ glm::vec3{ 1.f, 0.f, 0.f }, glm::vec3{ 0.f, -1.f, 0.f });
			break;
		case 1:
			m_uniformBufferCpu.m_viewMatrix = glm::lookAt(lv_lightPos, lv_lightPos + glm::vec3{ -1.f, 0.f, 0.f }, glm::vec3{ 0.f, -1.f, 0.f });
			break;
		case 2:
			m_uniformBufferCpu.m_viewMatrix = glm::lookAt(lv_lightPos, lv_lightPos + glm::vec3{ 0.f, 1.f, 0.f }, glm::vec3{ 0.f, 0.f, 1.f });
			break;
		case 3:
			m_uniformBufferCpu.m_viewMatrix = glm::lookAt(lv_lightPos, lv_lightPos + glm::vec3{ 0.f, -1.f, 0.f }, glm::vec3{ 0.f, 0.f, -1.f });
			break;
		case 4:
			m_uniformBufferCpu.m_viewMatrix = glm::lookAt(lv_lightPos, lv_lightPos + glm::vec3{ 0.f, 0.f, 1.f }, glm::vec3{ 0.f, -1.f, 0.f });
			break;
		case 5:
			m_uniformBufferCpu.m_viewMatrix = glm::lookAt(lv_lightPos, lv_lightPos + glm::vec3{ 0.f, 0.f, -1.f }, glm::vec3{ 0.f, -1.f, 0.f });
			break;
		}

		memcpy(lv_uniformBufferGpu.ptr, &m_uniformBufferCpu, sizeof(UniformBufferLight));*/

		
	}


	void DepthMapLightRenderer::UpdateDescriptorSets()
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();
		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();
		auto* lv_indirectRenderer = (IndirectRenderer*)lv_frameGraph.RetrieveNode("IndirectGbuffer")->m_renderer;

		auto lv_vertexBufferSize = lv_indirectRenderer->GetVertexBufferSize();

		std::vector<VkDescriptorBufferInfo> lv_bufferInfo;
		lv_bufferInfo.resize(lv_totalNumSwapchains*(4));

		for (size_t i = 0, j = 0; i < lv_bufferInfo.size(); i += 4, ++j) {

			lv_bufferInfo[i].buffer = lv_vkResManager.RetrieveGpuBuffer(m_uniformBufferGpuHandle).buffer;
			lv_bufferInfo[i].offset = 0;
			lv_bufferInfo[i].range = VK_WHOLE_SIZE;

			lv_bufferInfo[i + 1].buffer = lv_vkResManager.RetrieveGpuBuffer(m_indicesVerticesGpuBufferHandle).buffer;
			lv_bufferInfo[i + 1].offset = 0;
			lv_bufferInfo[i + 1].range = lv_vertexBufferSize;

			lv_bufferInfo[i + 2].buffer = lv_vkResManager.RetrieveGpuBuffer(m_indicesVerticesGpuBufferHandle).buffer;
			lv_bufferInfo[i + 2].offset = lv_vertexBufferSize;
			lv_bufferInfo[i + 2].range = lv_vkResManager.RetrieveGpuBuffer(m_indicesVerticesGpuBufferHandle).size - lv_vertexBufferSize;

			lv_bufferInfo[i + 3].buffer = m_instanceBuffersGpu[j]->buffer;
			lv_bufferInfo[i + 3].offset = 0;
			lv_bufferInfo[i + 3].range = VK_WHOLE_SIZE;

		}

		std::vector<VkWriteDescriptorSet> lv_writes{};
		lv_writes.resize(4 * lv_totalNumSwapchains);

		for (size_t i = 0, j = 0; i < lv_writes.size(); i += 4, ++j) {

			lv_writes[i].descriptorCount = 1;
			lv_writes[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			lv_writes[i].dstArrayElement = 0;
			lv_writes[i].dstBinding = 0;
			lv_writes[i].dstSet = m_descriptorSets[j];
			lv_writes[i].pBufferInfo = &lv_bufferInfo[i];
			lv_writes[i].pImageInfo = nullptr;
			lv_writes[i].pNext = nullptr;
			lv_writes[i].pTexelBufferView = nullptr;
			lv_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 1].descriptorCount = 1;
			lv_writes[i + 1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			lv_writes[i + 1].dstArrayElement = 0;
			lv_writes[i + 1].dstBinding = 1;
			lv_writes[i + 1].dstSet = m_descriptorSets[j];
			lv_writes[i + 1].pBufferInfo = &lv_bufferInfo[i + 1];
			lv_writes[i + 1].pImageInfo = nullptr;
			lv_writes[i + 1].pNext = nullptr;
			lv_writes[i + 1].pTexelBufferView = nullptr;
			lv_writes[i + 1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 2].descriptorCount = 1;
			lv_writes[i + 2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			lv_writes[i + 2].dstArrayElement = 0;
			lv_writes[i + 2].dstBinding = 2;
			lv_writes[i + 2].dstSet = m_descriptorSets[j];
			lv_writes[i + 2].pBufferInfo = &lv_bufferInfo[i + 2];
			lv_writes[i + 2].pImageInfo = nullptr;
			lv_writes[i + 2].pNext = nullptr;
			lv_writes[i + 2].pTexelBufferView = nullptr;
			lv_writes[i + 2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[i + 3].descriptorCount = 1;
			lv_writes[i + 3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			lv_writes[i + 3].dstArrayElement = 0;
			lv_writes[i + 3].dstBinding = 3;
			lv_writes[i + 3].dstSet = m_descriptorSets[j];
			lv_writes[i + 3].pBufferInfo = &lv_bufferInfo[i + 3];
			lv_writes[i + 3].pImageInfo = nullptr;
			lv_writes[i + 3].pNext = nullptr;
			lv_writes[i + 3].pTexelBufferView = nullptr;
			lv_writes[i + 3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		}

		vkUpdateDescriptorSets(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device, lv_writes.size(), lv_writes.data(), 0, nullptr);
	}

}