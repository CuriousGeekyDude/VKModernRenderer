#pragma once



#include "Renderbase.hpp"



namespace RenderCore
{

	class BloomBlendBlurAndSceneRenderer : public Renderbase
	{
	public:

		BloomBlendBlurAndSceneRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator
			, const char* l_vtxShader, const char* l_fragShader
			, const char* l_spv
			, const int l_blurTextureIndex);

		void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex) override;


		void UpdateBuffers(const uint32_t l_currentSwapchainIndex,
			const VulkanEngine::CameraStructure& l_cameraStructure) override;


		void UpdateDescriptorSets() override;


	private:
		std::vector<VulkanTexture*> m_swapchainTexture;
		std::vector<VulkanTexture*> m_deferredLightnintOutputTextures;
		std::vector<VulkanTexture*> m_gaussianBlurredTextures;

	};

}