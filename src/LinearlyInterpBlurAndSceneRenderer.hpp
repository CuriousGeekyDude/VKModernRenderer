#pragma once



#include "Renderbase.hpp"



namespace RenderCore
{

	class LinearlyInterpBlurAndSceneRenderer : public Renderbase
	{

		struct UniformBuffer
		{
			glm::vec4 m_mipchainDimensions{ 1.f };
			uint32_t m_indexMipchain{};
			float m_radius{};
			uint32_t m_pad1{};
			uint32_t m_pad2{};
		};

	public:




		LinearlyInterpBlurAndSceneRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator
			, const char* l_vtxShader, const char* l_fragShader
			, const char* l_spv);

		void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex) override;


		void UpdateBuffers(const uint32_t l_currentSwapchainIndex,
			const VulkanEngine::CameraStructure& l_cameraStructure) override;


		void UpdateDescriptorSets() override;


	private:
		std::vector<VulkanTexture*> m_mipMapInputImages;
		std::vector<VkImageView> m_descriptorImageViews{};
		std::vector <VulkanTexture*> m_outputImages{};
		
	};


}