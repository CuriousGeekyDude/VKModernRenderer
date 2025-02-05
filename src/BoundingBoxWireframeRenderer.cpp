


#include "BoundingBoxWireframeRenderer.hpp"
#include "GeometryConverter.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <format>
#include "CameraStructure.hpp"

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

		auto lv_boundingBoxData = m_vulkanRenderContext
			.GetCpuResourceProvider().RetrieveCpuResource("BoundingBoxData");
		CreateBoundingBoxVerticesAndIndices(lv_boundingBoxData);


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

		VkVertexInputBindingDescription lv_vertexBindingDescription{};
		lv_vertexBindingDescription.binding = 0;
		lv_vertexBindingDescription.stride = sizeof(glm::vec3);
		lv_vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription lv_vertexAttribDescription{};
		lv_vertexAttribDescription.binding = 0;
		lv_vertexAttribDescription.location = 0;
		lv_vertexAttribDescription.offset = 0;
		lv_vertexAttribDescription.format = VK_FORMAT_R32G32B32_SFLOAT;

		auto* lv_node = lv_frameGraph.RetrieveNode("BoundingBoxWireframe");
		VulkanResourceManager::PipelineInfo lv_pipelineInfo;
		lv_pipelineInfo.m_dynamicScissorState = false;
		lv_pipelineInfo.m_height = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferHeight;
		lv_pipelineInfo.m_width = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferWidth;
		lv_pipelineInfo.m_useBlending = false;
		lv_pipelineInfo.m_useDepth = true;
		lv_pipelineInfo.m_topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		lv_pipelineInfo.m_totalNumColorAttach = ((uint32_t)lv_node->m_outputResourcesHandles.size()) - 1;
		lv_pipelineInfo.m_vertexInputBindingDescription.push_back(lv_vertexBindingDescription);
		lv_pipelineInfo.m_vertexInputAttribDescription.push_back(lv_vertexAttribDescription);
		lv_pipelineInfo.m_enableWireframe = true;

		m_graphicsPipeline = lv_vkResManager.CreateGraphicsPipeline(m_renderPass, m_pipelineLayout,
			{ l_vertexShaderPath, l_fragmentShaderPath }, "BoundingBoxWireframeRendererPipeline",
			lv_pipelineInfo);


	}


	void BoundingBoxWireframeRenderer::UpdateUniformBuffers(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure)
	{
		auto lv_ratio = m_vulkanRenderContext.GetContextCreator()
			.m_vkDev.m_framebufferWidth / m_vulkanRenderContext.GetContextCreator()
			.m_vkDev.m_framebufferHeight;

		UniformBuffer lv_uniform{};
		lv_uniform.m_cameraPos = glm::vec4{ l_cameraStructure.m_cameraPos, 1.f };
		lv_uniform.m_viewMatrix = l_cameraStructure.m_viewMatrix;
		lv_uniform.m_inMtx = glm::perspective<float>(45.f, lv_ratio, 0.1f, 256.f) * l_cameraStructure.m_viewMatrix;


		auto& lv_uniformBufferGPU = m_vulkanRenderContext.GetResourceManager().RetrieveGpuBuffer(m_uniformBufferHandles[l_currentSwapchainIndex]);
		memcpy(lv_uniformBufferGPU.ptr, &lv_uniform, sizeof(UniformBuffer));
	}


	void BoundingBoxWireframeRenderer::FillCommandBuffer
	(VkCommandBuffer l_cmdBuffer, uint32_t l_currentSwapchainIndex)
	{
		auto lv_framebuffer = m_vulkanRenderContext.GetResourceManager().RetrieveGpuFramebuffer(m_framebufferHandles[l_currentSwapchainIndex]);
		auto& lv_indexBuffer = m_vulkanRenderContext.GetResourceManager().RetrieveGpuBuffer(m_indexBufferGpuHandle);
		auto& lv_vertexBuffer = m_vulkanRenderContext.GetResourceManager().RetrieveGpuBuffer(m_vertexBufferGpuHandle);
		auto lv_totalNumBoxes = m_boundingBoxVertices.size() / 24;
		auto& lv_swapchainTexture = m_vulkanRenderContext.GetResourceManager().RetrieveGpuTexture(m_swapchainHandles[l_currentSwapchainIndex]);
		auto& lv_depthTexture = m_vulkanRenderContext.GetResourceManager().RetrieveGpuTexture(m_depthTextureHandles[l_currentSwapchainIndex]);
		VkDeviceSize lv_offset = 0;

		vkCmdBindIndexBuffer(l_cmdBuffer, lv_indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdBindVertexBuffers(l_cmdBuffer, 0, 1, &lv_vertexBuffer.buffer, &lv_offset);

		transitionImageLayoutCmd(l_cmdBuffer, lv_depthTexture.image.image, lv_depthTexture.format,
			lv_depthTexture.Layout, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		lv_depthTexture.Layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		BeginRenderPass(m_renderPass, lv_framebuffer, l_cmdBuffer, l_currentSwapchainIndex, 1);
		for (size_t i = 0; i < lv_totalNumBoxes; ++i) {
			vkCmdDrawIndexed(l_cmdBuffer, m_boundingBoxIndices.size(), 1, 0, 8 * i, 0);
		}
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

			glm::vec3{0.f, 1.f, 0.f},
			glm::vec3{1.f, 1.f, 0.f},
			glm::vec3{0.f, 0.f, 0.f},
			glm::vec3{1.f, 0.f, 0.f},
			glm::vec3{1.f, 0.f, 1.f},
			glm::vec3{1.f, 1.f, 1.f},
			glm::vec3{0.f, 1.f, 1.f},
			glm::vec3{0.f, 0.f, 1.f},
		};

		m_boundingBoxVertices.resize(l_boundingBoxData.m_size * 24);
		m_boundingBoxIndices = {

			0, 1, 2,
			2, 1, 3,

			1, 5, 3,
			3, 5, 4,

			5, 6, 4,
			4, 6, 7,

			6, 0, 7,
			7, 0, 2,

			//up
			6, 1, 0,
			6, 5, 1,

			//down
			7, 3, 2,
			7, 4, 3
		};

		auto* lv_boundingBoxData = (MeshConverter::GeometryConverter::BoundingBox*)l_boundingBoxData.m_buffer;

		for (uint32_t i = 0, j = 0 ; i < (uint32_t)m_boundingBoxVertices.size(); i+=24, ++j) {
			
			auto& lv_min = lv_boundingBoxData[j].m_min;
			auto& lv_max = lv_boundingBoxData[j].m_max;

			auto lv_boxDimensions = lv_max - lv_min;

			for (uint32_t k = 0; k < 8; ++k) {
				
				auto& lv_corner = lv_unitCube[k];
				glm::vec3 lv_newCorner{lv_boxDimensions.x* lv_corner.x, lv_boxDimensions.y* lv_corner.y,
				lv_boxDimensions.z * lv_corner.z};

				glm::vec3 lv_boxVertex{lv_newCorner.x + lv_min.x, lv_newCorner.y + lv_min.y 
					,lv_newCorner.z + lv_min.z };

				m_boundingBoxVertices[i + 3*k] = lv_boxVertex.x;
				m_boundingBoxVertices[i + 3*k + 1] = lv_boxVertex.y;
				m_boundingBoxVertices[i + 3*k + 2] = lv_boxVertex.z;

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
		lv_bufferInfos.resize(lv_totalNumSwapchains);
		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
			auto& lv_uniformBuffer = lv_vkResManager.RetrieveGpuBuffer(m_uniformBufferHandles[i]);
			lv_bufferInfos[i].buffer = lv_uniformBuffer.buffer;
			lv_bufferInfos[i].offset = 0;
			lv_bufferInfos[i].range = lv_uniformBuffer.size;
		}

		std::vector<VkWriteDescriptorSet> lv_writes{};
		lv_writes.resize(lv_bufferInfos.size());

		for (size_t i = 0, j = 0; i < lv_writes.size() && j < lv_totalNumSwapchains; ++i, ++j) {
			lv_writes[i].descriptorCount = 1;
			lv_writes[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			lv_writes[i].dstArrayElement = 0;
			lv_writes[i].dstBinding = 0;
			lv_writes[i].dstSet = 0;
			lv_writes[i].pBufferInfo = &lv_bufferInfos[i];
			lv_writes[i].dstSet = m_descriptorSets[j];
			lv_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		}

		vkUpdateDescriptorSets(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device,
			lv_writes.size(), lv_writes.data(), 0, nullptr);
		
	}
}