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
		m_vulkanResources(m_vulkanContextCreator.m_vkDev),
		m_depth(m_vulkanResources.CreateDepthTexture(" Depth-Texture-Dummy ")),
		m_clearRenderPass(m_vulkanResources.CreateFullScreenRenderPass(true, {.clearColor_ = true, .clearDepth_ = true, .flags_ = eRenderPassBit_First_ColorAttach}, " Clear-RenderPass ")),
		m_clearRenderPassAfterFirst(m_vulkanResources.CreateFullScreenRenderPass(true, { .clearColor_ = true, .clearDepth_ = true, .flags_ = eRenderPassBit_First_ColorAttach_ColorAttach }," Clear-After-First-RenderPass ")),
		m_offScreenRenderPassDepth(m_vulkanResources.CreateFullScreenRenderPass(true, {.clearColor_ = true, .clearDepth_ = true, .flags_ = eRenderPassBit_First_ColorAttach_ColorAttach}, " Offscreen-Depth-RenderPass ")),
		m_presentRenderPass(m_vulkanResources.CreateFullScreenRenderPass(true, { .clearColor_ = false, .clearDepth_ = false, .flags_ = eRenderPassBit_Last }, " Present-RenderPass ")),
		m_offScreenRenderPassNoDepth(m_vulkanResources.CreateFullScreenRenderPass(false, { .clearColor_ = true, .clearDepth_ = false, .flags_ = eRenderPassBit_First_ColorAttach_ColorAttach }," Offscreen-No-Depth-RenderPass ")),
		m_offscreenContinueRenderPassDepth(m_vulkanResources.CreateFullScreenRenderPass(true, { .clearColor_ = false, .clearDepth_ = false, .flags_ = eRenderPassBit_ColorAttach_ColorAttach }, " Offscreen-Continue-RenderPass ")),
		m_presentToColorAttachRenderPassDepth(m_vulkanResources.CreateFullScreenRenderPass(true, { .clearColor_ = false, .clearDepth_ = false, .flags_ = eRenderPassBit_Present_ColorAttach }, " Present-To-Color-Attachment-RenderPass "))
		//m_colorAttachDepthAttachToShaderReadOnlyRenderpass(m_vulkanResources.CreateFullScreenRenderPass(true, { .clearColor_ = false, .clearDepth_ = false, .flags_ = eRenderPassBit_ColorAttach_ShaderReadOnly }))
	{
		m_swapchainFrameBuffersDepth = m_vulkanResources.CreateFullScreenFrameBuffers(m_offScreenRenderPassDepth, m_depth.image.imageView);
		m_swapchainFrameBuffersNoDepth = m_vulkanResources.CreateFullScreenFrameBuffers(m_offScreenRenderPassNoDepth, VK_NULL_HANDLE);
		m_swapchainFrameBufferDepthClear = m_vulkanResources.CreateFullScreenFrameBuffers(m_clearRenderPass, m_depth.image.imageView);
		m_swapchainFrameBufferDepthPresent = m_vulkanResources.CreateFullScreenFrameBuffers(m_presentRenderPass, m_depth.image.imageView);
		m_swapchainFrameBuffersDepthContinue = m_vulkanResources.CreateFullScreenFrameBuffers(m_offscreenContinueRenderPassDepth, m_depth.image.imageView);
		m_swapchainFrameBufferClearAfterFirst = m_vulkanResources.CreateFullScreenFrameBuffers(m_clearRenderPassAfterFirst, m_depth.image.imageView);
		m_swapchainFrameBufferPresentToColorAttach = m_vulkanResources.CreateFullScreenFrameBuffers(m_presentToColorAttachRenderPassDepth, m_depth.image.imageView);
		m_frameGraph.emplace(l_frameGraphJSONFile, *this);

	}

	std::vector<VkFramebuffer>& VulkanRenderContext::GetSwapchainFramebufferNoDepth() { return m_swapchainFrameBuffersNoDepth; }


	//RenderCore::VulkanResourceManager::RenderPass& VulkanRenderContext::GetColorAttachDepthAttachToShaderReadOnlyRenderpass() { return m_colorAttachDepthAttachToShaderReadOnlyRenderpass; }

	VulkanEngine::VulkanContextCreator& VulkanRenderContext::GetContextCreator() { return m_vulkanContextCreator; }
	RenderCore::VulkanResourceManager& VulkanRenderContext::GetResourceManager() { return m_vulkanResources; }

	RenderCore::VulkanResourceManager::RenderPass& VulkanRenderContext::GetOffscreenRenderPassDepth() { return m_offScreenRenderPassDepth; }
	RenderCore::VulkanResourceManager::RenderPass& VulkanRenderContext::GetOffscreenRenderPassNoDepth() { return m_offScreenRenderPassNoDepth; }

	RenderCore::VulkanResourceManager::RenderPass& VulkanRenderContext::GetClearRenderPass() { return m_clearRenderPass; }
	RenderCore::VulkanResourceManager::RenderPass& VulkanRenderContext::GetPresentRenderPass() { return m_presentRenderPass; }

	VkFramebuffer VulkanRenderContext::GetSwapchainFramebufferDepth(size_t l_swapchainIndex) { assert(l_swapchainIndex < m_vulkanContextCreator.m_vkDev.m_swapchainImages.size()); return m_swapchainFrameBuffersDepth[l_swapchainIndex]; }
	VkFramebuffer VulkanRenderContext::GetSwapchainFramebufferNoDepth(size_t l_swapchainIndex) { assert(l_swapchainIndex < m_vulkanContextCreator.m_vkDev.m_swapchainImages.size()); return m_swapchainFrameBuffersNoDepth[l_swapchainIndex]; }


	std::vector<VkFramebuffer>& VulkanRenderContext::GetSwapchainFramebufferDepth() { return m_swapchainFrameBuffersDepth; }


	FrameGraph& VulkanRenderContext::GetFrameGraph()
	{
		return m_frameGraph.value();
	}

	VulkanEngine::CpuResourceServiceProvider& VulkanRenderContext::GetCpuResourceProvider()
	{
		return m_cpuResourceProvider;
	}


	void VulkanRenderContext::UpdateUniformBuffers(uint32_t l_currentImageIndex,
		const CameraStructure& l_cameraStructure)
	{
		m_frameGraph.value().UpdateNodes(l_currentImageIndex, l_cameraStructure);
	}


	void VulkanRenderContext::UpdateRenderers(uint32_t L_currentImageIndex, 
		const CameraStructure& l_cameraStructure)
	{

		UpdateUniformBuffers(L_currentImageIndex, l_cameraStructure);
		
		for (auto& l_renderers : m_offScreenRenderers) {
			l_renderers.m_rendererBase.UpdateStorageBuffers(L_currentImageIndex);
		} 

	}


	RenderCore::VulkanResourceManager::RenderPass& VulkanRenderContext::GetOffscreenRenderPassDepthContinue()
	{
		return m_offscreenContinueRenderPassDepth;
	}

	void VulkanRenderContext::BeginRenderPass(VkCommandBuffer l_cmdBuffer, VkRenderPass l_renderpass,
		size_t l_currentImage, const VkRect2D l_area,
		VkFramebuffer l_framebuffer, uint32_t l_clearValueCount, const VkClearValue* l_clearValues)
	{
		VkRenderPassBeginInfo lv_renderPassBeginInfo{};
		lv_renderPassBeginInfo.clearValueCount = l_clearValueCount;
		lv_renderPassBeginInfo.framebuffer = (l_framebuffer != VK_NULL_HANDLE) ? l_framebuffer : m_swapchainFrameBuffersDepth[l_currentImage];
		lv_renderPassBeginInfo.pClearValues = l_clearValues;
		lv_renderPassBeginInfo.renderArea = l_area;
		lv_renderPassBeginInfo.renderPass = l_renderpass;
		lv_renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;


		vkCmdBeginRenderPass(l_cmdBuffer, &lv_renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	}


	void VulkanRenderContext::CreateFrame(VkCommandBuffer l_cmdBuffer, uint32_t l_currentImageIndex)
	{
		typedef RenderCore::VulkanResourceManager::RenderPass  RenderPass;

		static uint32_t lv_totalNumClearRenderPassUsed{};

		VkRect2D lv_screenRect{};
		lv_screenRect.extent = {.width = m_vulkanContextCreator.m_vkDev.m_framebufferWidth, 
			.height = m_vulkanContextCreator.m_vkDev.m_framebufferHeight};
		lv_screenRect.offset = { 0, 0 };

	  VkClearValue lv_clearValues[2] = {
	  VkClearValue {.color = {1.f, 1.f, 1.f, 1.f} },
	  VkClearValue {.depthStencil = {1.f, 0} }
		};

	  if (true == m_clearRenderPassUsed) {
		  BeginRenderPass(l_cmdBuffer, m_presentToColorAttachRenderPassDepth.m_renderpass, l_currentImageIndex,
			  lv_screenRect, m_swapchainFrameBufferPresentToColorAttach[l_currentImageIndex]);
		  vkCmdEndRenderPass(l_cmdBuffer);
		  /*BeginRenderPass(l_cmdBuffer, m_clearRenderPassAfterFirst.m_renderpass, l_currentImageIndex, lv_screenRect,
			  m_swapchainFrameBufferClearAfterFirst[l_currentImageIndex], 2, lv_clearValues);
		  vkCmdEndRenderPass(l_cmdBuffer);*/
		}
	  else {
		  BeginRenderPass(l_cmdBuffer, m_clearRenderPass.m_renderpass, l_currentImageIndex, lv_screenRect,
			  m_swapchainFrameBufferDepthClear[l_currentImageIndex], 2, lv_clearValues);
		  vkCmdEndRenderPass(l_cmdBuffer);
		  ++lv_totalNumClearRenderPassUsed;
		  if (m_vulkanContextCreator.m_vkDev.m_swapchainImageViews.size() == lv_totalNumClearRenderPassUsed) {
			  m_clearRenderPassUsed = true;
		  }
	  }
	  m_frameGraph.value().RenderGraph(l_cmdBuffer, l_currentImageIndex);
		BeginRenderPass(l_cmdBuffer, m_presentRenderPass.m_renderpass, l_currentImageIndex,
			lv_screenRect, m_swapchainFrameBufferDepthPresent[l_currentImageIndex]);
		vkCmdEndRenderPass(l_cmdBuffer);


	}
}