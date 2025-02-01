#pragma once



#include "Renderbase.hpp"

namespace RenderCore
{

	class VulkanClear : public Renderbase
	{

	public:

		VulkanClear(VulkanEngine::VulkanRenderContext& l_vkContextCreator);

		virtual void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex) override;

		virtual void UpdateUniformBuffers(const uint32_t l_currentSwapchainIndex,
			const VulkanEngine::CameraStructure& l_cameraStructure) override;

	protected:
		virtual void UpdateDescriptorSets() override;

	};

}