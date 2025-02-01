#pragma once


#include "VulkanRendererItem.hpp"
#include "Renderbase.hpp"
#include <vector>

namespace RenderCore
{
	class CompositeRenderer : public Renderbase
	{
	public:

		CompositeRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator);


		virtual void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex) override;

		virtual void UpdateUniformBuffers(const uint32_t l_currentSwapchainIndex,
			const VulkanEngine::CameraStructure& l_cameraStructure) override;



	protected:
		virtual void UpdateDescriptorSets() override;


	protected:
		std::vector<VulkanRendererItem> m_renderers;
	};
}