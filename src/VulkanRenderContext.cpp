#pragma once



#include "VulkanRenderContext.hpp"
#include "Renderbase.hpp"
#include <array>
#include "CameraStructure.hpp"


namespace VulkanEngine
{
	VulkanRenderContext::VulkanRenderContext(void* l_window, 
		uint32_t l_screenWidth, uint32_t l_screenHeight, const std::string& l_frameGraphJSONFile)
		: m_vulkanContextCreator(l_window, l_screenWidth, l_screenHeight),
		m_vulkanResources(m_vulkanContextCreator.m_vkDev)
		,m_fullScreenHeight(l_screenHeight)
		,m_fullScreenWidth(l_screenWidth)
	{
		m_frameGraph.emplace(l_frameGraphJSONFile, *this);

	}

	uint32_t VulkanRenderContext::GetFullScreenWidth() const
	{
		return m_fullScreenWidth;
	}
	uint32_t VulkanRenderContext::GetFullScreenHeight() const
	{
		return m_fullScreenHeight;
	}

	VulkanEngine::VulkanContextCreator& VulkanRenderContext::GetContextCreator() { return m_vulkanContextCreator; }
	RenderCore::VulkanResourceManager& VulkanRenderContext::GetResourceManager() { return m_vulkanResources; }

	FrameGraph& VulkanRenderContext::GetFrameGraph()
	{
		return m_frameGraph.value();
	}

	VulkanEngine::CpuResourceServiceProvider& VulkanRenderContext::GetCpuResourceProvider()
	{
		return m_cpuResourceProvider;
	}


	void VulkanRenderContext::UpdateBuffers(uint32_t l_currentImageIndex,
		const CameraStructure& l_cameraStructure)
	{
		m_frameGraph.value().UpdateNodes(l_currentImageIndex, l_cameraStructure);
	}


	void VulkanRenderContext::UpdateRenderers(uint32_t L_currentImageIndex, 
		const CameraStructure& l_cameraStructure)
	{

		UpdateBuffers(L_currentImageIndex, l_cameraStructure);
		
		for (auto& l_renderers : m_offScreenRenderers) {
			l_renderers.m_rendererBase.UpdateStorageBuffers(L_currentImageIndex);
		} 

	}




	void VulkanRenderContext::BeginRenderPass(VkCommandBuffer l_cmdBuffer, VkRenderPass l_renderpass,
		size_t l_currentImage, const VkRect2D l_area,
		VkFramebuffer l_framebuffer, uint32_t l_clearValueCount, const VkClearValue* l_clearValues)
	{
		VkRenderPassBeginInfo lv_renderPassBeginInfo{};
		lv_renderPassBeginInfo.clearValueCount = l_clearValueCount;
		lv_renderPassBeginInfo.framebuffer = l_framebuffer;
		lv_renderPassBeginInfo.pClearValues = l_clearValues;
		lv_renderPassBeginInfo.renderArea = l_area;
		lv_renderPassBeginInfo.renderPass = l_renderpass;
		lv_renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;


		vkCmdBeginRenderPass(l_cmdBuffer, &lv_renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	}


	void VulkanRenderContext::CreateFrame(VkCommandBuffer l_cmdBuffer, uint32_t l_currentImageIndex)
	{
		//typedef RenderCore::VulkanResourceManager::RenderPass  RenderPass;

		//static uint32_t lv_totalNumClearRenderPassUsed{};

		//VkRect2D lv_screenRect{};
		//lv_screenRect.extent = {.width = m_vulkanContextCreator.m_vkDev.m_framebufferWidth, 
		//	.height = m_vulkanContextCreator.m_vkDev.m_framebufferHeight};
		//lv_screenRect.offset = { 0, 0 };

	 // VkClearValue lv_clearValues[2] = {
	 // VkClearValue {.color = {1.f, 1.f, 1.f, 1.f} },
	 // VkClearValue {.depthStencil = {1.f, 0} }
		//};

	 // if (true == m_clearRenderPassUsed) {
		//  BeginRenderPass(l_cmdBuffer, m_presentToColorAttachRenderPassDepth.m_renderpass, l_currentImageIndex,
		//	  lv_screenRect, m_swapchainFrameBufferPresentToColorAttach[l_currentImageIndex]);
		//  vkCmdEndRenderPass(l_cmdBuffer);
		//  /*BeginRenderPass(l_cmdBuffer, m_clearRenderPassAfterFirst.m_renderpass, l_currentImageIndex, lv_screenRect,
		//	  m_swapchainFrameBufferClearAfterFirst[l_currentImageIndex], 2, lv_clearValues);
		//  vkCmdEndRenderPass(l_cmdBuffer);*/
		//}
	 // else {
		//  BeginRenderPass(l_cmdBuffer, m_clearRenderPass.m_renderpass, l_currentImageIndex, lv_screenRect,
		//	  m_swapchainFrameBufferDepthClear[l_currentImageIndex], 2, lv_clearValues);
		//  vkCmdEndRenderPass(l_cmdBuffer);
		//  ++lv_totalNumClearRenderPassUsed;
		//  if (m_vulkanContextCreator.m_vkDev.m_swapchainImageViews.size() == lv_totalNumClearRenderPassUsed) {
		//	  m_clearRenderPassUsed = true;
		//  }
	 // }
	  m_frameGraph.value().RenderGraph(l_cmdBuffer, l_currentImageIndex);


	}
}