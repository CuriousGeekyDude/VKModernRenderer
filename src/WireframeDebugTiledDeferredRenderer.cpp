



#include "WireframeDebugTiledDeferredRenderer.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include "UtilsMath.h"
#include "CameraStructure.hpp"


namespace RenderCore
{


	WireframeDebugTiledDeferredRenderer::WireframeDebugTiledDeferredRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator
								, const char* l_vtxShader, const char* l_fragShader, const char* l_spv)
		:Renderbase(l_vkContextCreator)
	{
		
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();
		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();

		constexpr uint32_t lv_totalNumLights = 256;


		m_grid.resize(32*32*8);

		uint32_t lv_vertexBufferHandle = lv_vkResManager.CreateBufferWithHandle(sizeof(glm::vec4) * lv_totalNumLights * 4, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
			, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "VertexBufferWireframeDebugTiledDeferred");
		uint32_t lv_indexBufferHandle = lv_vkResManager.CreateBufferWithHandle(sizeof(uint16_t) * 8, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
			, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "IndexBufferWireframeDebugTiledDeferred");

		uint32_t lv_vertexBufferGridVerticalHandle = lv_vkResManager.CreateBufferWithHandle(sizeof(glm::vec4) * 66, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
			, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "VertexBufferGridVerticalWireframeDebugTiledDeferred");
		uint32_t lv_vertexBufferGridHorizontalHandle = lv_vkResManager.CreateBufferWithHandle(sizeof(glm::vec4) * 66, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
			, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "VertexBufferGridHorizontalWireframeDebugTiledDeferred");
		uint32_t lv_indexBufferGridHandle = lv_vkResManager.CreateBufferWithHandle(2*sizeof(uint16_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
			, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "IndexBufferGridWireframeDebugTiledDeferred");

		m_vertexBufferGpu = &lv_vkResManager.RetrieveGpuBuffer(lv_vertexBufferHandle);
		m_indexBufferGpu = &lv_vkResManager.RetrieveGpuBuffer(lv_indexBufferHandle);
		m_vertexBufferGridHorizontalGpu = &lv_vkResManager.RetrieveGpuBuffer(lv_vertexBufferGridHorizontalHandle);
		m_vertexBufferGridVerticalGpu = &lv_vkResManager.RetrieveGpuBuffer(lv_vertexBufferGridVerticalHandle);
		m_indexBufferGridGpu = &lv_vkResManager.RetrieveGpuBuffer(lv_indexBufferGridHandle);

		std::array<glm::vec4, 66> lv_gridVertices{};
		for (size_t i = 0, j = 0; i < 66; i+=2, ++j) {
			float lv_x = -1.f + ((float)j) / 16.f;
			lv_gridVertices[i] = glm::vec4{lv_x, -1.f, 0.f, 0.f};
			lv_gridVertices[i+1] = glm::vec4{lv_x, 1.f, 0.f, 0.f};
		}
		lv_vkResManager.CopyDataToLocalBuffer(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_mainQueue2,
			m_vulkanRenderContext.GetContextCreator().m_vkDev.m_mainCommandBuffer1[0], lv_vertexBufferGridVerticalHandle,
			lv_gridVertices.data());

		for (size_t i = 0, j = 0; i < 66; i += 2, ++j) {
			float lv_y = -1.f + ((float)j) / 16.f;
			lv_gridVertices[i] = glm::vec4{ -1.f , lv_y, 0.f, 0.f };
			lv_gridVertices[i + 1] = glm::vec4{ 1.f, lv_y, 0.f, 0.f };
		}
		lv_vkResManager.CopyDataToLocalBuffer(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_mainQueue2,
			m_vulkanRenderContext.GetContextCreator().m_vkDev.m_mainCommandBuffer1[0], lv_vertexBufferGridHorizontalHandle,
			lv_gridVertices.data());


		std::array<uint16_t, 2> lv_indicesGrid{};
		lv_indicesGrid[0] = 0;
		lv_indicesGrid[1] = 1;
		lv_vkResManager.CopyDataToLocalBuffer(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_mainQueue2,
			m_vulkanRenderContext.GetContextCreator().m_vkDev.m_mainCommandBuffer1[0], lv_indexBufferGridHandle,
			lv_indicesGrid.data());


		m_indicesBufferCpu = {0, 1, 1,2,2, 3,3, 0};


		m_depthTextures.resize(lv_totalNumSwapchains);
		m_swapchainTextures.resize(lv_totalNumSwapchains);
		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
			m_depthTextures[i] = &lv_vkResManager.RetrieveGpuTexture("Depth", i);
			m_swapchainTextures[i] = &lv_vkResManager.RetrieveGpuTexture("Swapchain", i);
		}

		lv_vkResManager.CopyDataToLocalBuffer(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_mainQueue2,
			m_vulkanRenderContext.GetContextCreator().m_vkDev.m_mainCommandBuffer1[0], lv_indexBufferHandle,
			m_indicesBufferCpu.data());

		m_lights.resize(2);
		
		m_lights[0].m_color = glm::vec4{1.f, 1.f, 1.f, 1.f};
		m_lights[0].m_min = glm::vec4{0.f};
		m_lights[0].m_max = glm::vec4{0.f};
		m_lights[0].m_max += glm::vec4{ 5.090127f ,5.090127f ,5.090127f , 1.f };
		m_lights[0].m_min += glm::vec4{ -5.090127f ,-5.090127f ,-5.090127f , 1.f };
		m_lights[0].m_worldTransform = glm::translate(glm::identity<glm::mat4>() , glm::vec3{ -13.f, 18.f, -2.f });
		m_lights[0].m_max = m_lights[0].m_worldTransform * m_lights[0].m_max;
		m_lights[0].m_min = m_lights[0].m_worldTransform * m_lights[0].m_min;
		

		m_lights[1].m_color = glm::vec4{ 1.f };
		m_lights[1].m_min = glm::vec4{0.f};
		m_lights[1].m_max = glm::vec4{0.f};
		m_lights[1].m_max += glm::vec4{ 5.090127f ,5.090127f ,5.090127f , 1.f };
		m_lights[1].m_min += glm::vec4{ -5.090127f ,-5.090127f ,-5.090127f , 1.f };
		m_lights[1].m_worldTransform = glm::translate(glm::identity<glm::mat4>(), glm::vec3{ -20.f, 18.f, -2.f });
		m_lights[1].m_max = m_lights[1].m_worldTransform * m_lights[1].m_max;
		m_lights[1].m_min = m_lights[1].m_worldTransform * m_lights[1].m_min;
		


		constexpr uint32_t lv_totalNumBins{ 32 };
		m_sortedLights.resize(m_lights.size());
		m_sortedIndicesOfLight.resize(m_lights.size());
		m_bins.resize(lv_totalNumBins);
		m_verticesBufferCpu.resize(4 * m_lights.size());
		m_lightAABBIntersectionTestFrustum.resize(m_lights.size());

		SetRenderPassAndFrameBuffer("WireframeDebugTiledDeferred");
		SetNodeToAppropriateRenderpass("WireframeDebugTiledDeferred", this);
		lv_frameGraph.IncrementNumNodesPerCmdBuffer(2);

		m_pipelineLayout = lv_vkResManager.CreatePipelineLayout(m_descriptorSetLayout, "WireframeDebugTiledDeferredPipelineLayout");

		VkVertexInputBindingDescription lv_vtxBindingDesc1{};
		lv_vtxBindingDesc1.binding = 0;
		lv_vtxBindingDesc1.stride = sizeof(glm::vec4);
		lv_vtxBindingDesc1.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription lv_vtxAttribDesc10{};
		lv_vtxAttribDesc10.binding = 0;
		lv_vtxAttribDesc10.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		lv_vtxAttribDesc10.location = 0;
		lv_vtxAttribDesc10.offset = 0;



		auto* lv_node = lv_frameGraph.RetrieveNode("WireframeDebugTiledDeferred");
		VulkanResourceManager::PipelineInfo lv_pipelineInfo;
		lv_pipelineInfo.m_dynamicScissorState = false;
		lv_pipelineInfo.m_height = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferHeight;
		lv_pipelineInfo.m_width = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferWidth;
		lv_pipelineInfo.m_useBlending = false;
		lv_pipelineInfo.m_useDepth = true;
		lv_pipelineInfo.m_enableWireframe = true;
		lv_pipelineInfo.m_topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		lv_pipelineInfo.m_totalNumColorAttach = ((uint32_t)lv_node->m_outputResourcesHandles.size()) - 1;

		lv_pipelineInfo.m_vertexInputBindingDescription.push_back(lv_vtxBindingDesc1);
		lv_pipelineInfo.m_vertexInputAttribDescription.push_back(lv_vtxAttribDesc10);

		m_graphicsPipeline = lv_vkResManager.CreateGraphicsPipeline(m_renderPass, m_pipelineLayout,
			{ l_vtxShader, l_fragShader }, "WireframeDebugTiledDeferredRendererPipeline",
			lv_pipelineInfo);

	}


	void WireframeDebugTiledDeferredRenderer::FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
		uint32_t l_currentSwapchainIndex)
	{
		auto lv_framebuffer = m_vulkanRenderContext.GetResourceManager().RetrieveGpuFramebuffer(m_framebufferHandles[l_currentSwapchainIndex]);

		std::vector<VulkanTexture*> lv_depthLayouts{ m_depthTextures[l_currentSwapchainIndex] };
		TransitionImageLayoutsCmd(l_cmdBuffer, lv_depthLayouts, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		VkDeviceSize lv_offset = 0;
		vkCmdBindIndexBuffer(l_cmdBuffer, m_indexBufferGpu->buffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdBindVertexBuffers(l_cmdBuffer, 0, 1, &m_vertexBufferGpu->buffer, &lv_offset);

		BeginRenderPass(m_renderPass, lv_framebuffer, l_cmdBuffer, l_currentSwapchainIndex, 1);
		for (size_t i = 0; i < m_sortedIndicesOfLight.size(); ++i) {
			vkCmdDrawIndexed(l_cmdBuffer, m_indicesBufferCpu.size(), 1, 0, 4 * m_sortedIndicesOfLight[i], 0);
		}

		vkCmdBindIndexBuffer(l_cmdBuffer, m_indexBufferGridGpu->buffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdBindVertexBuffers(l_cmdBuffer, 0, 1, &m_vertexBufferGridVerticalGpu->buffer, &lv_offset);

		for (size_t i = 0; i < 33; ++i) {
			vkCmdDrawIndexed(l_cmdBuffer, 2, 1, 0, 2*i, 0);
		}
		vkCmdBindVertexBuffers(l_cmdBuffer, 0, 1, &m_vertexBufferGridHorizontalGpu->buffer, &lv_offset);

		for (size_t i = 0; i < 33; ++i) {
			vkCmdDrawIndexed(l_cmdBuffer, 2, 1, 0, 2 * i, 0);
		}

		vkCmdEndRenderPass(l_cmdBuffer);
		
		m_swapchainTextures[l_currentSwapchainIndex]->Layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;



	}


	void WireframeDebugTiledDeferredRenderer::UpdateBuffers(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure)
	{
		auto lv_screenWidth = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferWidth;
		auto lv_screenHeight = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferHeight;
		memset(m_bins.data(), 0, sizeof(uint32_t) * m_bins.size());
		memset(m_verticesBufferCpu.data(), 0, sizeof(glm::vec4)*m_verticesBufferCpu.size());
		for (size_t i = 0; i < m_lights.size(); ++i) {
			m_sortedIndicesOfLight[i] = (uint32_t)i;
		}


		memset(m_grid.data(), 0, sizeof(uint32_t) * m_grid.size());

		auto lv_mvp = l_cameraStructure.m_projectionMatrix * l_cameraStructure.m_viewMatrix;


		

		for (size_t i = 0; i < m_lights.size(); ++i) {

			m_sortedLights[i].m_minView =   m_lights[i].m_min;
			m_sortedLights[i].m_maxView =  m_lights[i].m_max;
			/*m_sortedLights[i].m_minView += glm::vec4{ -5.090127f ,-5.090127f ,-5.090127f , 0.f };
			m_sortedLights[i].m_maxView += glm::vec4{ 5.090127f ,5.090127f ,5.090127f , 0.f };*/

			auto lv_difference = m_sortedLights[i].m_maxView - m_sortedLights[i].m_minView;
			lv_difference.x = std::abs(lv_difference.x);
			lv_difference.y = std::abs(lv_difference.y);
			lv_difference.z = std::abs(lv_difference.z);
			auto& lv_minView = m_sortedLights[i].m_minView;
			auto& lv_maxView = m_sortedLights[i].m_maxView;
			m_sortedLights[i].m_aabbView[0] = l_cameraStructure.m_viewMatrix * m_sortedLights[i].m_minView;
			m_sortedLights[i].m_aabbView[1] = l_cameraStructure.m_viewMatrix * glm::vec4{lv_minView.x + lv_difference.x, lv_minView.y, lv_minView.z, 1.f};
			m_sortedLights[i].m_aabbView[2] = l_cameraStructure.m_viewMatrix * glm::vec4{ lv_minView.x , lv_minView.y + lv_difference.y, lv_minView.z, 1.f };
			m_sortedLights[i].m_aabbView[3] = l_cameraStructure.m_viewMatrix * glm::vec4{ lv_minView.x , lv_minView.y , lv_minView.z + lv_difference.z, 1.f };
			m_sortedLights[i].m_aabbView[4] = l_cameraStructure.m_viewMatrix * glm::vec4{ lv_minView.x + lv_difference.x, lv_minView.y , lv_minView.z + lv_difference.z, 1.f };
			m_sortedLights[i].m_aabbView[5] = l_cameraStructure.m_viewMatrix * glm::vec4{ lv_maxView.x, lv_maxView.y, lv_maxView.z, 1.f };
			m_sortedLights[i].m_aabbView[6] = l_cameraStructure.m_viewMatrix * glm::vec4{ lv_minView.x + lv_difference.x, lv_minView.y + lv_difference.y, lv_minView.z , 1.f };
			m_sortedLights[i].m_aabbView[7] = l_cameraStructure.m_viewMatrix * glm::vec4{ lv_minView.x , lv_minView.y + lv_difference.y, lv_minView.z + lv_difference.z, 1.f };

			m_sortedLights[i].m_posView = l_cameraStructure.m_viewMatrix * (lv_minView + (lv_difference/2.f));
			m_sortedLights[i].m_minView = l_cameraStructure.m_viewMatrix * m_sortedLights[i].m_minView;
			m_sortedLights[i].m_maxView = l_cameraStructure.m_viewMatrix* m_sortedLights[i].m_maxView;
		}

		auto SortLightBasedOnZ = [this](const uint32_t l_a, const uint32_t l_b) -> bool
			{
				return m_sortedLights[l_a].m_posView.z > m_sortedLights[l_b].m_posView.z;
			};

		std::sort(m_sortedIndicesOfLight.begin(), m_sortedIndicesOfLight.end(), SortLightBasedOnZ);

		constexpr float lv_totalNumBins{ 31.f };
		constexpr float lv_farPlane{ 145.f };
		constexpr float lv_nearPlane{ 0.1f };
		const float lv_denominator{ std::log10f(lv_farPlane / lv_nearPlane) };
		const float lv_logNear{std::log10f(lv_nearPlane)};
		const float lv_secondTerm{ (lv_totalNumBins * lv_logNear / lv_denominator) };
		const float lv_firstTerm{ (lv_totalNumBins / lv_denominator) };
		auto DetermineBin = [&lv_firstTerm, &lv_secondTerm](const float l_zView) -> uint32_t
			{
				
				return (uint32_t)std::floorf( (std::log10f(l_zView) * lv_firstTerm)   -   lv_secondTerm );
			};


		for (size_t i = 0; i < m_bins.size(); ++i) {

			float lv_a = lv_nearPlane*(std::powf(lv_farPlane/lv_nearPlane, ((float)i)/((float)m_bins.size())));
			float lv_b = lv_nearPlane * (std::powf(lv_farPlane / lv_nearPlane, ((float)(i+1)) / ((float)m_bins.size())));
			uint32_t lv_min{ std::numeric_limits<uint32_t>::max() };
			uint32_t lv_max{ 0 };
			float lv_newMinViewZ{std::numeric_limits<float>::max()};
			float lv_newMaxViewZ{-std::numeric_limits<float>::max() +1000.f};

			for (size_t j = 0; j < m_sortedIndicesOfLight.size(); ++j) {

				auto lv_sortedLightIndex = m_sortedIndicesOfLight[j];



				for (auto& l_vertexView : m_sortedLights[lv_sortedLightIndex].m_aabbView) {
					if (lv_newMinViewZ > l_vertexView.z) {
						lv_newMinViewZ = l_vertexView.z;
					}
					if (lv_newMaxViewZ < l_vertexView.z) {
						lv_newMaxViewZ = l_vertexView.z;
					}
				}



				/*auto lv_posViewZMapped = (-1.f * m_sortedLights[m_sortedIndicesOfLight[j]].m_posView.z - 0.01f) / 999.99f;
				auto lv_minViewZMapped = (-1.f * m_sortedLights[m_sortedIndicesOfLight[j]].m_minView.z - 0.01f) / 999.99f;
				auto lv_maxViewZMapped = (-1.f * m_sortedLights[m_sortedIndicesOfLight[j]].m_maxView.z - 0.01f) / 999.99f;*/


				if (lv_newMinViewZ >= -0.1f || lv_newMaxViewZ <= -145.f) {
					continue;
				}

				

				if (lv_b >= -lv_newMaxViewZ && lv_a <= -lv_newMinViewZ) {

					if (lv_min > (uint32_t)j) {
						lv_min = (uint32_t)j;
					}
					if (lv_max < (uint32_t)j) {
						lv_max = (uint32_t)j;
					}
				}
				
			}

			m_bins[i] = lv_min | (lv_max << 16);
		}


		

		for (size_t i = 0; i < m_sortedIndicesOfLight.size(); ++i) {

			auto lv_sortedLightIndex = m_sortedIndicesOfLight[i];

			auto& lv_posView = m_sortedLights[lv_sortedLightIndex].m_posView;

			if (glm::dot(glm::vec3{ lv_posView.x,lv_posView.y,lv_posView.z}, glm::vec3{ lv_posView.x,lv_posView.y,lv_posView.z }) > 270.f) {
				std::array<glm::vec4, 8> lv_projVertices{};
				for (size_t j = 0; j < 8; ++j) {
					lv_projVertices[j] = l_cameraStructure.m_projectionMatrix * m_sortedLights[lv_sortedLightIndex].m_aabbView[j];
					lv_projVertices[j] = (1.f / lv_projVertices[j].w) * lv_projVertices[j];
				}

				glm::vec2 lv_projMax{ (-1.f * std::numeric_limits<float>::max()) - 1000.f };
				glm::vec2 lv_projMin{ std::numeric_limits<float>::max() };
				for (size_t j = 0; j < 8; ++j) {
					if (lv_projMax.x < lv_projVertices[j].x) {
						lv_projMax.x = lv_projVertices[j].x;
					}
					if (lv_projMax.y < lv_projVertices[j].y) {
						lv_projMax.y = lv_projVertices[j].y;
					}
					if (lv_projMin.x > lv_projVertices[j].x) {
						lv_projMin.x = lv_projVertices[j].x;
					}
					if (lv_projMin.y > lv_projVertices[j].y) {
						lv_projMin.y = lv_projVertices[j].y;
					}
				}

				//if (lv_projMax.x <= -1 && lv_projMax.y <= -1) {
				//	lv_projMax = glm::vec2{};
				//	lv_projMin = glm::vec2{};
				//}

				//else if (lv_projMin.x >= 1 && lv_projMin.y >= 1) {
				//	lv_projMin = glm::vec2{};
				//	lv_projMax = glm::vec2{};
				//}
				//else if ((lv_projMin.x <= -1.f && lv_projMax.x >= 1) || (lv_projMin.x <= -1.f && lv_projMax.y >= 1.f) || (lv_projMin.y <= -1.f && lv_projMax.x >= 1.f) || (lv_projMin.y <= -1.f && lv_projMax.y >= 1.f)) {
				//	lv_projMin = glm::vec2{};
				//	lv_projMax = glm::vec2{};
				//}
				//else {
				//	//4 cases completely outside of the rectangle
				//	if (lv_projMax.y <= -1.f || lv_projMin.y >= 1.f || lv_projMax.x <= -1.f || lv_projMin.x >= 1.f) {
				//		lv_projMin = glm::vec2{};
				//		lv_projMax = glm::vec2{};
				//	}

				//	else {
				//		//4 cases partially inside the rectangle
				//		if (lv_projMax.x >= 1.f) {
				//			lv_projMax.x = 1.f;
				//		}
				//		if (lv_projMax.y >= 1) {
				//			lv_projMax.y = 1.f;
				//		}
				//		if (lv_projMin.x <= -1.f) {
				//			lv_projMin.x = -1.f;
				//		}
				//		if (lv_projMin.y <= -1.f) {
				//			lv_projMin.y = -1.f;
				//		}
				//	}
				//}


				auto deltaTemp = lv_projMax - lv_projMin;
				glm::vec2 lv_a = glm::vec2{ lv_projMin.x,deltaTemp.y + lv_projMin.y };
				glm::vec2 lv_b = glm::vec2{ lv_projMin.x + deltaTemp.x , lv_projMin.y };
				if ((-1.f <= lv_projMax.x && lv_projMax.x <= 1.f && -1.f <= lv_projMax.y && lv_projMax.y <= 1.f) ||
					(-1.f <= lv_projMin.x && lv_projMin.x <= 1.f && -1.f <= lv_projMin.y && lv_projMin.y <= 1.f) ||
					(-1.f <= lv_a.x && lv_a.x <= 1.f && -1.f <= lv_a.y && lv_a.y <= 1.f) ||
					(-1.f <= lv_b.x && lv_b.x <= 1.f && -1.f <= lv_b.y && lv_b.y <= 1.f)) {
					if (lv_projMax.x >= 1.f) {
						lv_projMax.x = 1.f;
					}
					if (lv_projMax.y >= 1) {
						lv_projMax.y = 1.f;
					}
					if (lv_projMin.x <= -1.f) {
						lv_projMin.x = -1.f;
					}
					if (lv_projMin.y <= -1.f) {
						lv_projMin.y = -1.f;
					}


					auto lv_delta = lv_projMax - lv_projMin;
					lv_delta.x = std::abs(lv_delta.x);
					lv_delta.y = std::abs(lv_delta.y);
					m_verticesBufferCpu[4 * lv_sortedLightIndex] = glm::vec4{ lv_projMin, 0.f, 1.f };
					m_verticesBufferCpu[4 * lv_sortedLightIndex + 1] = glm::vec4{ lv_projMin.x + lv_delta.x, lv_projMin.y, 0.f, 1.f };
					m_verticesBufferCpu[4 * lv_sortedLightIndex + 2] = glm::vec4{ lv_projMax, 0.f, 1.f };
					m_verticesBufferCpu[4 * lv_sortedLightIndex + 3] = glm::vec4{ lv_projMin.x, lv_projMin.y + lv_delta.y, 0.f, 1.f };


					glm::vec2 lv_screenSpaceMin = (glm::vec2{ (float)(lv_screenWidth) * (0.5f * lv_projMin.x + 0.5f), ((float)(lv_screenHeight)) * (0.5f * lv_projMin.y + 0.5f) });
					glm::vec2 lv_screenSpaceMax = (glm::vec2{ (float)(lv_screenWidth) * (0.5f * lv_projMax.x + 0.5f), ((float)(lv_screenHeight)) * (0.5f * lv_projMax.y + 0.5f) });

					uint32_t lv_firstXMultiple = (uint32_t)((lv_screenSpaceMin.x * 32.f) / ((float)lv_screenWidth));
					uint32_t lv_firstYMultiple = (uint32_t)((lv_screenSpaceMin.y * 32.f) / ((float)lv_screenHeight));

					uint32_t lv_lastXMultiple = (uint32_t)((lv_screenSpaceMax.x * 32.f) / ((float)lv_screenWidth));
					uint32_t lv_lastYMultiple = (uint32_t)((lv_screenSpaceMax.y * 32.f) / ((float)lv_screenHeight));

					lv_firstXMultiple = std::clamp(lv_firstXMultiple, (uint32_t)0, (uint32_t)31);
					lv_firstYMultiple = std::clamp(lv_firstYMultiple, (uint32_t)0, (uint32_t)32);
					lv_lastXMultiple = std::clamp(lv_lastXMultiple, (uint32_t)0, (uint32_t)31);
					lv_lastYMultiple = std::clamp(lv_lastYMultiple, (uint32_t)0, (uint32_t)32);


					constexpr uint32_t lv_multiple{ 31 * 8 };

					for (uint32_t i = lv_firstYMultiple; i <= lv_lastYMultiple; ++i) {
						for (uint32_t j = lv_firstXMultiple; j <= lv_lastXMultiple; ++j) {

							auto lv_currentLightIndex = m_sortedIndicesOfLight[lv_sortedLightIndex];

							uint32_t lv_orderOfNumber = lv_currentLightIndex / 32;
							uint32_t lv_bitPlace = lv_currentLightIndex % 32;
							m_grid.at(i * lv_multiple + j * 8 + lv_orderOfNumber) |= (1 << lv_bitPlace);
						}
					}
				}
				else {
					m_verticesBufferCpu[4 * lv_sortedLightIndex] = glm::vec4{ 0.f, 0.f, 0.f, 1.f };
					m_verticesBufferCpu[4 * lv_sortedLightIndex + 1] = glm::vec4{ 0.f, 0.f, 0.f, 1.f };
					m_verticesBufferCpu[4 * lv_sortedLightIndex + 2] = glm::vec4{ 0.f, 0.f, 0.f, 1.f };
					m_verticesBufferCpu[4 * lv_sortedLightIndex + 3] = glm::vec4{ 0.f, 0.f, 0.f, 1.f };
				}
			}
			else {
				m_verticesBufferCpu[4 * lv_sortedLightIndex] = glm::vec4{ -1.f, -1.f, 0.f, 1.f };
				m_verticesBufferCpu[4 * lv_sortedLightIndex + 1] = glm::vec4{ 1.f, -1.f, 0.f, 1.f };
				m_verticesBufferCpu[4 * lv_sortedLightIndex + 2] = glm::vec4{ 1.f, 1.f, 0.f, 1.f };
				m_verticesBufferCpu[4 * lv_sortedLightIndex + 3] = glm::vec4{ -1.f, 1.f, 0.f, 1.f };
			}
			
			
		}

		memcpy(m_vertexBufferGpu->ptr, m_verticesBufferCpu.data(), sizeof(glm::vec4) * m_verticesBufferCpu.size());

	}



	void WireframeDebugTiledDeferredRenderer::UpdateDescriptorSets()
	{

	}

}