#pragma once


#include "Renderbase.hpp"
#include <imgui/imgui.h>

namespace RenderCore
{
	class ImGuiRenderer : public Renderbase
	{

	public:

		explicit ImGuiRenderer(VulkanRenderDevice& vkDev);
		explicit ImGuiRenderer(VulkanRenderDevice& vkDev, const std::vector<VulkanTexture>& textures);
		virtual ~ImGuiRenderer();

		virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t currentImage, VkFramebuffer l_frameBuffer = VK_NULL_HANDLE,
			VkRenderPass l_renderpass = VK_NULL_HANDLE) override;
		void updateBuffers(VulkanRenderDevice& vkDev, uint32_t currentImage, const ImDrawData* imguiDrawData);

	private:
		const ImDrawData* drawData = nullptr;

		bool createDescriptorSet(VulkanRenderDevice& vkDev);

		/* Descriptor set with multiple textures (for offscreen buffer display etc.) */
		bool createMultiDescriptorSet(VulkanRenderDevice& vkDev);

		std::vector<VulkanTexture> extTextures_;

		// storage buffer with index and vertex data
		VkDeviceSize bufferSize_;
		std::vector<VkBuffer> storageBuffer_;
		std::vector<VkDeviceMemory> storageBufferMemory_;

		VkSampler fontSampler_;
		VulkanImage font_;
	};
}