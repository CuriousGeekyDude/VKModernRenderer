#pragma once






#include "Renderbase.hpp"



namespace RenderCore
{

	class GaussianBlurRenderer :public Renderbase
	{

	public:

		GaussianBlurRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator
							, const char* l_vtxShader, const char* l_fragShader
							, const char* l_spv
							, const int l_blurTextureIndex
							, const bool l_referenceSceneOutputTexture
							, const char* l_nodeName);

		void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex) override;


		
		
		
		void UpdateBuffers(const uint32_t l_currentSwapchainIndex,
			const VulkanEngine::CameraStructure& l_cameraStructure) override;
			

		void UpdateDescriptorSets() override;



	private:
		std::vector<VulkanTexture*> m_blurOutputTextures;
		std::vector<VulkanTexture*> m_inputSampleTextures;

	};
}
