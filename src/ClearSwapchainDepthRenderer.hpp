#pragma once






#include "Renderbase.hpp"


namespace RenderCore
{

	class ClearSwapchainDepthRenderer : public Renderbase
	{

	public:

		ClearSwapchainDepthRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator);

		void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex) override;

		void UpdateBuffers(const uint32_t l_currentSwapchainIndex,
			const VulkanEngine::CameraStructure& l_cameraStructure) override;

		void UpdateDescriptorSets() override;

	private:

		std::vector<VulkanTexture*> m_swapchainTextures;
		std::vector<VulkanTexture*> m_depthTextures;


	};


}