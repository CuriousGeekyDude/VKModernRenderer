#pragma once





#include "Renderbase.hpp"


namespace RenderCore
{
	class ExtractBrightnessRenderer : public Renderbase
	{
	public:

		ExtractBrightnessRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator
								, const char* l_vtxShader, const char* l_fragShader
								, const char* l_spv);
		void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex) override;

		void UpdateBuffers(const uint32_t l_currentSwapchainIndex,
			const VulkanEngine::CameraStructure& l_cameraStructure) override;

		void UpdateDescriptorSets() override;

	private:
		std::vector<VulkanTexture*> m_extractedBrightnessTextures;
		std::vector<VulkanTexture*> m_colorOutputTextures;

	};
}