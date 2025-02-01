#pragma once


#include "Renderbase.hpp"


namespace RenderCore
{

	class QuadRenderer : public Renderbase
	{
	public:

		QuadRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator,
			const std::string& l_vulkanDebugNameRenderer,
			const RenderCore::VulkanResourceManager::RenderPass& l_renderpass,
			const RenderCore::VulkanResourceManager::PipelineInfo& l_pipelineInfo,
			const std::vector<VkFramebuffer>& l_frameBuffers,
			std::vector<VulkanTexture>* l_frameBufferTextures,
			const std::vector<RenderCore::VulkanResourceManager::DescriptorSetResources>& l_descriptorSetInfos,
			const std::vector<const char*>& l_shaders);

		virtual void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex);

	private:

		std::vector<VulkanTexture>* m_frameBufferTextures;
	};

}