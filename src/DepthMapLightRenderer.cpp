



#include "DepthMapLightRenderer.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "IndirectRenderer.hpp"

namespace RenderCore
{
	

	DepthMapLightRenderer::DepthMapLightRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator
		, const char* l_vtxShader, const char* l_fragShader
		, const char* l_spvFile, const glm::vec4& l_lightPos)
		:Renderbase(l_vkContextCreator)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();
		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();
		auto* lv_indirectRenderer = (IndirectRenderer*)lv_frameGraph.RetrieveNode("IndirectGbuffer")->m_renderer;

		m_uniformBufferCpu.m_pos = glm::rotate(glm::identity<glm::mat4>(), glm::radians(-30.f), glm::vec3{1.f, 0.f,0.f}) * l_lightPos;
		m_uniformBufferCpu.m_viewMatrix = glm::lookAt(glm::vec3{l_lightPos.x, l_lightPos.y, l_lightPos.z}, glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
		m_uniformBufferCpu.m_orthoMatrix = glm::ortho(-80.0f, 80.0f, -80.0f, 80.0f, 0.1f, 70.f);

		glm::mat4 lv_iden = glm::identity<glm::mat4>();
		lv_iden[2][2] = 0.5f;
		lv_iden[3][2] = 0.5f;
		m_uniformBufferCpu.m_orthoMatrix = lv_iden * m_uniformBufferCpu.m_orthoMatrix;

		m_uniformBufferGpuHandle = lv_vkResManager.CreateBufferWithHandle(sizeof(UniformBufferLight), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
																		 , VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
																		 , "UniformBufferLightMatricesDepthMap");
		m_depthMapGpuTextures.resize(lv_totalNumSwapchains);
		m_instanceBuffersGpu.resize(lv_totalNumSwapchains);
		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
			m_depthMapGpuTextures[i] = &lv_vkResManager.RetrieveGpuTexture("DepthMapLightTexture", i);
			m_instanceBuffersGpu[i] = &lv_vkResManager.RetrieveGpuBuffer("Instance-Buffer-Indirect", i);
		}



		auto& lv_outputInstanceData = lv_indirectRenderer->GetInstanceData();
		auto& lv_meshes = lv_indirectRenderer->GetMeshData();

		m_indirectBufferGpuHandle = lv_vkResManager.CreateBufferWithHandle(sizeof(VkDrawIndirectCommand) * lv_outputInstanceData.size()
																		  , VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
																		  ,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
																		  , "indirectBufferDepthMapLight");

		auto& lv_indirectBuffer = m_vulkanRenderContext.GetResourceManager().RetrieveGpuBuffer(m_indirectBufferGpuHandle);
		VkDrawIndirectCommand* lv_drawStructure = (VkDrawIndirectCommand*)lv_indirectBuffer.ptr;
		for (uint32_t i = 0; i < lv_outputInstanceData.size(); ++i) {
			auto j = lv_outputInstanceData[i].m_meshIndex;
			lv_drawStructure[i].vertexCount = lv_meshes[j].CalculateLODNumberOfIndices(lv_outputInstanceData[i].m_lod);
			lv_drawStructure[i].firstInstance = i;
			lv_drawStructure[i].firstVertex = 0U;
			lv_drawStructure[i].instanceCount = 1;

		}



		auto& lv_uniformBufferGpu = lv_vkResManager.RetrieveGpuBuffer(m_uniformBufferGpuHandle);
		memcpy(lv_uniformBufferGpu.ptr, &m_uniformBufferCpu, sizeof(UniformBufferLight));


		auto lv_indicesVerticesMetaData = lv_vkResManager.RetrieveGpuResourceMetaData(" Vertex-Buffer-Indirect ");

		assert(std::numeric_limits<float>::max() != lv_indicesVerticesMetaData.m_resourceHandle);
		m_indicesVerticesGpuBufferHandle = lv_indicesVerticesMetaData.m_resourceHandle;

		GeneratePipelineFromSpirvBinaries(l_spvFile);
		SetRenderPassAndFrameBuffer("DepthMapLight");
		SetNodeToAppropriateRenderpass("DepthMapLight", this);
		UpdateDescriptorSets();

		auto* lv_node = lv_frameGraph.RetrieveNode("DepthMapLight");
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


		m_graphicsPipeline = lv_vkResManager.CreateGraphicsPipeline(m_renderPass, m_pipelineLayout
			, { l_vtxShader, l_fragShader }, "GraphicsPipelineDepthMapLight", lv_pipeInfo);
	}

	void DepthMapLightRenderer::FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
		uint32_t l_currentSwapchainIndex)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();

		auto* lv_currDepthMapTex = m_depthMapGpuTextures[l_currentSwapchainIndex];

		transitionImageLayoutCmd(l_cmdBuffer, lv_currDepthMapTex->image.image, lv_currDepthMapTex->format
								, lv_currDepthMapTex->Layout, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		lv_currDepthMapTex->Layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


		auto& lv_indirectBuffer = lv_vkResManager.RetrieveGpuBuffer(m_indirectBufferGpuHandle);
		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();
		auto* lv_indirectRenderer = (IndirectRenderer*)lv_frameGraph.RetrieveNode("IndirectGbuffer")->m_renderer;
		auto lv_totalNumInstances = lv_indirectRenderer->GetInstanceData().size();

		auto lv_framebuffer = lv_vkResManager.RetrieveGpuFramebuffer(m_framebufferHandles[l_currentSwapchainIndex]);

		BeginRenderPass(m_renderPass, lv_framebuffer, l_cmdBuffer, l_currentSwapchainIndex, 1);
		vkCmdDrawIndirect(l_cmdBuffer, lv_indirectBuffer.buffer, 0, lv_totalNumInstances,
			sizeof(VkDrawIndirectCommand));
		vkCmdEndRenderPass(l_cmdBuffer);

		lv_currDepthMapTex->Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	}

	void DepthMapLightRenderer::UpdateBuffers(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure)
	{

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