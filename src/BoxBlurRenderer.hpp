#pragma once



#include "Renderbase.hpp"


namespace RenderCore
{

	class BoxBlurRenderer :public Renderbase
	{
	public:

		BoxBlurRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator
						, const char* l_vtxShader, const char* l_fragmentShader
						, const char* l_spvFile);

		void UpdateDescriptorSets() override;

		void UpdateBuffers(const uint32_t l_currentSwapchainIndex,
			const VulkanEngine::CameraStructure& l_cameraStructure) override;

		void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex) override;

	private:
		std::vector<VulkanTexture*> m_frameBufferTextures;
		std::vector<VulkanTexture*> m_ssaoTextures;

	};

}