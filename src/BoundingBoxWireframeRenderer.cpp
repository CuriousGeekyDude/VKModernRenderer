


#include "BoundingBoxWireframeRenderer.hpp"
#include "GeometryConverter.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <format>
#include "CameraStructure.hpp"
#include "UtilsMath.h"
#include <GLFW/glfw3.h>

namespace RenderCore
{
	BoundingBoxWireframeRenderer::BoundingBoxWireframeRenderer
	(VulkanEngine::VulkanRenderContext& l_vkContextCreator,
		const char* l_vertexShaderPath, const char* l_fragmentShaderPath,
		const char* l_spirvPath)
		:Renderbase(l_vkContextCreator)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();
		auto lv_ratio = (float)m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferWidth
			/ (float)m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferHeight;

		auto lv_boundingBoxData = m_vulkanRenderContext
			.GetCpuResourceProvider().RetrieveCpuResource("BoundingBoxData");
		CreateBoundingBoxVerticesAndIndices(lv_boundingBoxData);


		m_debugViewFrustum.m_indexBuffer = {
			0,1, 1,2, 2,3, 3,0, 0,4, 4,7, 7,3, 4,5, 5,6, 6,7, 2,6, 1,5
		};
		m_debugViewFrustum.m_projectionMatrix = glm::perspective<float>((float)glm::radians(60.f)
			, lv_ratio, 0.01f, 1000.f);
		m_debugViewFrustum.m_viewMatrix = glm::lookAt(glm::vec3(0.f, 2.f, 0.f),
			glm::vec3(0.f, 2.f, -0.5f), glm::vec3(0.f, 1.f, 0.f));
		getFrustumPlanes(m_debugViewFrustum.m_projectionMatrix * m_debugViewFrustum.m_viewMatrix,
			m_debugViewFrustum.m_debugViewFrustumPlanes);
		getFrustumCorners(m_debugViewFrustum.m_projectionMatrix * m_debugViewFrustum.m_viewMatrix
			, m_debugViewFrustum.m_debugViewFrustumCorners);

		m_debugViewFrustumVertexGpuHandle = lv_vkResManager.CreateBufferWithHandle(8 * sizeof(glm::vec4),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			"DebugViewFrustumVertexBuffer");
		m_debugViewFrustumIndexGpuHandle = lv_vkResManager.CreateBufferWithHandle(sizeof(uint16_t) * 24,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			"DebugViewFrustumIndexBuffer");
		auto& lv_debuViewVertexBufferGpu = lv_vkResManager.RetrieveGpuBuffer(m_debugViewFrustumVertexGpuHandle);
		auto& lv_debugViewIndexBufferGpu = lv_vkResManager.RetrieveGpuBuffer(m_debugViewFrustumIndexGpuHandle);
		memcpy(lv_debuViewVertexBufferGpu.ptr, m_debugViewFrustum.m_debugViewFrustumCorners.data(), 8*sizeof(glm::vec3));
		memcpy(lv_debugViewIndexBufferGpu.ptr, m_debugViewFrustum.m_indexBuffer.data(), 24*sizeof(uint16_t));

		m_depthTextureHandles.resize(lv_totalNumSwapchains);
		m_swapchainHandles.resize(lv_totalNumSwapchains);
		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
			auto lv_formattedArg = std::make_format_args(i);

			std::string lv_formattedString = "Depth {}";
			auto depthMeta = lv_vkResManager.RetrieveGpuResourceMetaData(std::vformat(lv_formattedString, lv_formattedArg));
			if (depthMeta.m_resourceHandle == UINT32_MAX) {
				printf("Depth texture was not found. Exitting....");
				exit(-1);
			}

			m_depthTextureHandles[i] = depthMeta.m_resourceHandle;

			lv_formattedString = "Swapchain {}";
			auto lv_swapchainMeta = lv_vkResManager.RetrieveGpuResourceMetaData(std::vformat(lv_formattedString, lv_formattedArg));
			m_swapchainHandles[i] = lv_swapchainMeta.m_resourceHandle;
		}

		auto lv_totalNumWireframeObjects = m_boundingBoxVertices.size() / 32;
		++lv_totalNumWireframeObjects;
		m_colorIndicesOfWireframes.resize(lv_totalNumWireframeObjects);
		m_colorIndicesOfWireframes[lv_totalNumWireframeObjects - 1] = 2;
		m_colorIndicesOfWireframesGpuHandle = lv_vkResManager.CreateBufferWithHandle(sizeof(uint32_t) * lv_totalNumWireframeObjects
			, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			"ColorIndicesOfWireframesBuffer");
		auto& lv_colorIndicesBufferGpu = lv_vkResManager.RetrieveGpuBuffer(m_colorIndicesOfWireframesGpuHandle);
		memcpy(lv_colorIndicesBufferGpu.ptr, m_colorIndicesOfWireframes.data(), sizeof(uint32_t)*lv_totalNumWireframeObjects);
		

		GeneratePipelineFromSpirvBinaries(l_spirvPath);
		SetRenderPassAndFrameBuffer("BoundingBoxWireframe");

		SetNodeToAppropriateRenderpass("BoundingBoxWireframe",this);

		m_uniformBufferHandles.resize(lv_totalNumSwapchains);
		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
			m_uniformBufferHandles[i] = lv_vkResManager.CreateBufferWithHandle(sizeof(UniformBuffer),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				std::format("BoundingBoxUniformBuffer {}", i).c_str());
		}

		

		UpdateDescriptorSets();

		m_pipelineLayout = lv_vkResManager.CreatePipelineLayout(m_descriptorSetLayout, "BoundingBoxWireframePipelineLayout");

		/*VkVertexInputBindingDescription lv_vertexBindingDescription{};
		lv_vertexBindingDescription.binding = 0;
		lv_vertexBindingDescription.stride = sizeof(glm::vec3);
		lv_vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription lv_vertexAttribDescription{};
		lv_vertexAttribDescription.binding = 0;
		lv_vertexAttribDescription.location = 0;
		lv_vertexAttribDescription.offset = 0;
		lv_vertexAttribDescription.format = VK_FORMAT_R32G32B32_SFLOAT;*/

		VkVertexInputBindingDescription lv_vtxBindingDesc1{};
		lv_vtxBindingDesc1.binding = 0;
		lv_vtxBindingDesc1.stride = sizeof(glm::vec4);
		lv_vtxBindingDesc1.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription lv_vtxAttribDesc10{};
		lv_vtxAttribDesc10.binding = 0;
		lv_vtxAttribDesc10.format = VK_FORMAT_R32G32B32_SFLOAT;
		lv_vtxAttribDesc10.location = 0;
		lv_vtxAttribDesc10.offset = 0;

		VkVertexInputAttributeDescription lv_vtxAttribDesc11{};
		lv_vtxAttribDesc11.binding = 0;
		lv_vtxAttribDesc11.format = VK_FORMAT_R32_SFLOAT;
		lv_vtxAttribDesc11.location = 1;
		lv_vtxAttribDesc11.offset = sizeof(glm::vec3);

		auto* lv_node = lv_frameGraph.RetrieveNode("BoundingBoxWireframe");
		VulkanResourceManager::PipelineInfo lv_pipelineInfo;
		lv_pipelineInfo.m_dynamicScissorState = false;
		lv_pipelineInfo.m_height = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferHeight;
		lv_pipelineInfo.m_width = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferWidth;
		lv_pipelineInfo.m_useBlending = false;
		lv_pipelineInfo.m_useDepth = true;
		lv_pipelineInfo.m_topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		lv_pipelineInfo.m_totalNumColorAttach = ((uint32_t)lv_node->m_outputResourcesHandles.size()) - 1;

		lv_pipelineInfo.m_vertexInputBindingDescription.push_back(lv_vtxBindingDesc1);

		lv_pipelineInfo.m_vertexInputAttribDescription.push_back(lv_vtxAttribDesc10);
		lv_pipelineInfo.m_vertexInputAttribDescription.push_back(lv_vtxAttribDesc11);



		lv_pipelineInfo.m_enableWireframe = true;

		m_graphicsPipeline = lv_vkResManager.CreateGraphicsPipeline(m_renderPass, m_pipelineLayout,
			{ l_vertexShaderPath, l_fragmentShaderPath }, "BoundingBoxWireframeRendererPipeline",
			lv_pipelineInfo);


	}



	void BoundingBoxWireframeRenderer::ApplyDebugCPUFrustumCulling()
	{
		auto lv_boundingBoxMeta = m_vulkanRenderContext
			.GetCpuResourceProvider().RetrieveCpuResource("BoundingBoxData");

		auto* lv_boundingBoxData = (MeshConverter::GeometryConverter::BoundingBox*)lv_boundingBoxMeta.m_buffer;

		for (size_t i = 0; i < lv_boundingBoxMeta.m_size; ++i) {
			if (true == isBoxInFrustum(m_debugViewFrustum.m_debugViewFrustumPlanes
				, m_debugViewFrustum.m_debugViewFrustumCorners
				, lv_boundingBoxData[i])) {
				m_colorIndicesOfWireframes[i] = 1;
			}
			else {
				m_colorIndicesOfWireframes[i] = 0;
			}
		}
	}

	void BoundingBoxWireframeRenderer::UpdateUniformBuffers(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure)
	{

		UniformBuffer lv_uniform{};
		lv_uniform.m_cameraPos = glm::vec4{ l_cameraStructure.m_cameraPos, 1.f };
		lv_uniform.m_viewMatrix = l_cameraStructure.m_viewMatrix;
		lv_uniform.m_inMtx = l_cameraStructure.m_projectionMatrix * l_cameraStructure.m_viewMatrix;


		
		/*auto lv_rotationMat = glm::rotate(glm::mat4(1.f), 0.5f*(float)glfwGetTime(), glm::vec3{0.f, 1.f, 0.f});
		auto lv_rotationResult = lv_rotationMat * glm::vec4{0.f, 2.25f, -0.5f, 1.f};

		m_debugViewFrustum.m_viewMatrix = glm::lookAt(glm::vec3(0.f, 55.f, 0.f),
			glm::vec3{ lv_rotationResult.x, lv_rotationResult.y, lv_rotationResult.z}, glm::vec3(0.f, 1.f, 0.f));*/

		
		m_debugViewFrustum.m_viewMatrix = l_cameraStructure.m_viewMatrix;
	


		auto lv_mvp = m_debugViewFrustum.m_projectionMatrix * m_debugViewFrustum.m_viewMatrix;
		getFrustumCorners(lv_mvp,m_debugViewFrustum.m_debugViewFrustumCorners);
		
		getFrustumPlanes(lv_mvp, m_debugViewFrustum.m_debugViewFrustumPlanes);
		ApplyDebugCPUFrustumCulling();

		
		
		std::array<glm::vec4, 8> lv_newCorners{};
		auto lv_totalNumWireframes = m_boundingBoxVertices.size() / 32;
		++lv_totalNumWireframes;
		for (size_t i = 0; i < 8; ++i) {
			lv_newCorners[i] = glm::vec4{ m_debugViewFrustum.m_debugViewFrustumCorners[i], lv_totalNumWireframes-1};
		}
		m_colorIndicesOfWireframes[lv_totalNumWireframes-1] = 2;
		auto& lv_debugViewVertexBuffer = m_vulkanRenderContext.GetResourceManager().RetrieveGpuBuffer(m_debugViewFrustumVertexGpuHandle);
		memcpy(lv_debugViewVertexBuffer.ptr, &lv_newCorners, sizeof(glm::vec4)*8);

		auto& lv_uniformBufferGPU = m_vulkanRenderContext.GetResourceManager().RetrieveGpuBuffer(m_uniformBufferHandles[l_currentSwapchainIndex]);
		memcpy(lv_uniformBufferGPU.ptr, &lv_uniform, sizeof(UniformBuffer));

		auto& lv_colorIndicesBufferGpu = m_vulkanRenderContext.GetResourceManager().RetrieveGpuBuffer(m_colorIndicesOfWireframesGpuHandle);
		memcpy(lv_colorIndicesBufferGpu.ptr, m_colorIndicesOfWireframes.data()
			,(uint32_t)sizeof(uint32_t)*m_colorIndicesOfWireframes.size());
	}


	void BoundingBoxWireframeRenderer::FillCommandBuffer
	(VkCommandBuffer l_cmdBuffer, uint32_t l_currentSwapchainIndex)
	{
		auto lv_framebuffer = m_vulkanRenderContext.GetResourceManager().RetrieveGpuFramebuffer(m_framebufferHandles[l_currentSwapchainIndex]);
		auto& lv_indexBuffer = m_vulkanRenderContext.GetResourceManager().RetrieveGpuBuffer(m_indexBufferGpuHandle);
		auto& lv_vertexBuffer = m_vulkanRenderContext.GetResourceManager().RetrieveGpuBuffer(m_vertexBufferGpuHandle);
		auto lv_totalNumBoxes = m_boundingBoxVertices.size() / 32;
		auto& lv_swapchainTexture = m_vulkanRenderContext.GetResourceManager().RetrieveGpuTexture(m_swapchainHandles[l_currentSwapchainIndex]);
		auto& lv_depthTexture = m_vulkanRenderContext.GetResourceManager().RetrieveGpuTexture(m_depthTextureHandles[l_currentSwapchainIndex]);
		VkDeviceSize lv_offset = 0;

		auto& lv_debugViewIndexBuffer = m_vulkanRenderContext.GetResourceManager().RetrieveGpuBuffer(m_debugViewFrustumIndexGpuHandle);
		auto& lv_debugViewVertexBuffer = m_vulkanRenderContext.GetResourceManager().RetrieveGpuBuffer(m_debugViewFrustumVertexGpuHandle);

		vkCmdBindIndexBuffer(l_cmdBuffer, lv_indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdBindVertexBuffers(l_cmdBuffer, 0, 1, &lv_vertexBuffer.buffer, &lv_offset);

		transitionImageLayoutCmd(l_cmdBuffer, lv_depthTexture.image.image, lv_depthTexture.format,
			lv_depthTexture.Layout, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		lv_depthTexture.Layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		BeginRenderPass(m_renderPass, lv_framebuffer, l_cmdBuffer, l_currentSwapchainIndex, 1);
		for (size_t i = 0; i < lv_totalNumBoxes; ++i) {
			vkCmdDrawIndexed(l_cmdBuffer, m_boundingBoxIndices.size(), 1, 0, 8 * i, 0);
		}
		vkCmdBindIndexBuffer(l_cmdBuffer, lv_debugViewIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdBindVertexBuffers(l_cmdBuffer,0, 1, &lv_debugViewVertexBuffer.buffer, &lv_offset );
		vkCmdDrawIndexed(l_cmdBuffer, 24, 1, 0, 0, 0);
		vkCmdEndRenderPass(l_cmdBuffer);
		lv_swapchainTexture.Layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	}


	void BoundingBoxWireframeRenderer::CreateBoundingBoxVerticesAndIndices
				(const VulkanEngine::CpuResource& l_boundingBoxData)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();

		if (l_boundingBoxData.m_buffer == nullptr) {
			printf("Bounding box data is empty. Exitting...\n");
			exit(-1);
		}

		

		constexpr std::array<glm::vec3, 8> lv_unitCube{

			glm::vec3{0.0f, 0.0f, 0.0f},
			glm::vec3{1.0f, 0.0f, 0.0f},
			glm::vec3{1.0f, 1.0f, 0.0f},
			glm::vec3{0.0f, 1.0f, 0.0f},
			glm::vec3{0.0f, 0.0f, 1.0f},
			glm::vec3{1.0f, 0.0f, 1.0f},
			glm::vec3{1.0f, 1.0f, 1.0f},
			glm::vec3{0.0f, 1.0f, 1.0f},
		};

		m_boundingBoxVertices.resize(l_boundingBoxData.m_size * 32);
		m_boundingBoxIndices = {

			0, 1,  1, 2,  2, 3,  3, 0,
			// Top face edges
			4, 5,  5, 6,  6, 7,  7, 4,
			// Vertical edges connecting top and bottom
			0, 4,  1, 5,  2, 6,  3, 7
		};

		auto* lv_boundingBoxData = (MeshConverter::GeometryConverter::BoundingBox*)l_boundingBoxData.m_buffer;

		for (uint32_t i = 0, j = 0 ; i < (uint32_t)m_boundingBoxVertices.size(); i+=32, ++j) {
			
			auto& lv_min = lv_boundingBoxData[j].m_min;
			auto& lv_max = lv_boundingBoxData[j].m_max;

			auto lv_boxDimensions = lv_max - lv_min;

			for (uint32_t k = 0; k < 8; ++k) {
				
				auto& lv_corner = lv_unitCube[k];
				glm::vec3 lv_newCorner{lv_boxDimensions.x* lv_corner.x, lv_boxDimensions.y* lv_corner.y,
				lv_boxDimensions.z * lv_corner.z};

				glm::vec3 lv_boxVertex{lv_newCorner.x + lv_min.x, lv_newCorner.y + lv_min.y 
					,lv_newCorner.z + lv_min.z };

				m_boundingBoxVertices[i + 4*k] = lv_boxVertex.x;
				m_boundingBoxVertices[i + 4*k + 1] = lv_boxVertex.y;
				m_boundingBoxVertices[i + 4*k + 2] = lv_boxVertex.z;
				m_boundingBoxVertices[i + 4 * k + 3] = j;

			}

		}

		m_vertexBufferGpuHandle = lv_vkResManager.CreateBufferWithHandle(sizeof(float) * m_boundingBoxVertices.size(),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			"BoundingBoxVertexBuffer");
		m_indexBufferGpuHandle = lv_vkResManager.CreateBufferWithHandle(sizeof(uint16_t) * m_boundingBoxIndices.size(),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			"BoundingBoxIndexBuffer");

		lv_vkResManager.CopyDataToLocalBuffer(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_mainQueue2,
			m_vulkanRenderContext.GetContextCreator().m_vkDev.m_mainCommandBuffer1[0], m_vertexBufferGpuHandle,
			m_boundingBoxVertices.data());
		lv_vkResManager.CopyDataToLocalBuffer(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_mainQueue2,
			m_vulkanRenderContext.GetContextCreator().m_vkDev.m_mainCommandBuffer1[0], m_indexBufferGpuHandle,
			m_boundingBoxIndices.data());

	}


	void BoundingBoxWireframeRenderer::UpdateDescriptorSets()
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();

		std::vector<VkDescriptorBufferInfo> lv_bufferInfos;
		lv_bufferInfos.resize(2*lv_totalNumSwapchains);
		for (size_t i = 0, j = 0; i < lv_bufferInfos.size(); i+=2, ++j) {
			auto& lv_uniformBuffer = lv_vkResManager.RetrieveGpuBuffer(m_uniformBufferHandles[j]);
			lv_bufferInfos[i].buffer = lv_uniformBuffer.buffer;
			lv_bufferInfos[i].offset = 0;
			lv_bufferInfos[i].range = lv_uniformBuffer.size;

			auto& lv_colorIndicesBuffer = lv_vkResManager.RetrieveGpuBuffer(m_colorIndicesOfWireframesGpuHandle);
			lv_bufferInfos[i + 1].buffer = lv_colorIndicesBuffer.buffer;
			lv_bufferInfos[i + 1].offset = 0;
			lv_bufferInfos[i + 1].range = lv_colorIndicesBuffer.size;
		}

		std::vector<VkWriteDescriptorSet> lv_writes{};
		lv_writes.resize(lv_bufferInfos.size());

		for (size_t i = 0, j = 0; i < lv_writes.size() && j < lv_totalNumSwapchains; i+=2, ++j) {
			lv_writes[i].descriptorCount = 1;
			lv_writes[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			lv_writes[i].dstArrayElement = 0;
			lv_writes[i].dstBinding = 0;
			lv_writes[i].dstSet = 0;
			lv_writes[i].pBufferInfo = &lv_bufferInfos[i];
			lv_writes[i].dstSet = m_descriptorSets[j];
			lv_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;


			lv_writes[i+1].descriptorCount = 1;
			lv_writes[i+1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			lv_writes[i+1].dstArrayElement = 0;
			lv_writes[i+1].dstBinding = 1;
			lv_writes[i+1].dstSet = 0;
			lv_writes[i+1].pBufferInfo = &lv_bufferInfos[i+1];
			lv_writes[i+1].dstSet = m_descriptorSets[j];
			lv_writes[i+1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		}

		vkUpdateDescriptorSets(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device,
			lv_writes.size(), lv_writes.data(), 0, nullptr);
		
	}
}