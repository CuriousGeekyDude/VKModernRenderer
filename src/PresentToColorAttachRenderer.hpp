#pragma once





#include "Renderbase.hpp"



namespace RenderCore
{


	class PresentToColorAttachRenderer : public Renderbase
	{

	public:

		PresentToColorAttachRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator);

		void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex) override;

		void UpdateBuffers(const uint32_t l_currentSwapchainIndex,
			const VulkanEngine::CameraStructure& l_cameraStructure) override;

		void UpdateDescriptorSets() override;

	private:
		
	};


}