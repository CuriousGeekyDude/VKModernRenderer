#pragma once


#include "UtilsVulkan.h"
#include "VulkanRendererItem.hpp"
#include "VulkanResourceManager.hpp"
#include "VulkanContextCreator.hpp"
#include "CpuResourceServiceProvider.hpp"
#include <vector>
#include <cassert>
#include "FrameGraph.hpp"
#include <optional>

namespace VulkanEngine
{

	struct CameraStructure;


	class VulkanRenderContext
	{

	public:

		VulkanRenderContext(void* l_window, uint32_t l_screenWidth, uint32_t l_screenHeight,
			const std::string& l_frameGraphJSONFile);

		void UpdateRenderers(uint32_t L_currentImageIndex, const CameraStructure& l_cameraStructure);


		void BeginRenderPass(VkCommandBuffer l_cmdBuffer, VkRenderPass l_renderpass,
			size_t l_currentImageIndex, const VkRect2D l_area,
			VkFramebuffer l_framebuffer = VK_NULL_HANDLE,
			uint32_t l_clearValueCount = 0, const VkClearValue* l_clearValues = nullptr);


		void CreateFrame(VkCommandBuffer l_cmdBuffer, uint32_t l_currentImageIndex);

		VulkanEngine::VulkanContextCreator& GetContextCreator();
		RenderCore::VulkanResourceManager& GetResourceManager();


		std::vector<VkFramebuffer>& GetSwapchainFramebufferDepth();

		
		std::vector<VkFramebuffer>& GetSwapchainFramebufferNoDepth();

		FrameGraph& GetFrameGraph();

		VulkanEngine::CpuResourceServiceProvider& GetCpuResourceProvider();

	public:

		std::vector<RenderCore::VulkanRendererItem> m_offScreenRenderers;


	private:

		void UpdateBuffers(uint32_t L_currentImageIndex, const CameraStructure& l_cameraStructure);

	private:

		bool m_clearRenderPassUsed{ false };

		VulkanEngine::VulkanContextCreator m_vulkanContextCreator;
		RenderCore::VulkanResourceManager m_vulkanResources;
		VulkanEngine::CpuResourceServiceProvider m_cpuResourceProvider;
		std::optional<FrameGraph> m_frameGraph;

	};
}