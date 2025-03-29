#pragma once



#include "Renderbase.hpp"


namespace RenderCore
{


	class UpsampleBlendRenderer : public Renderbase
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

		UpsampleBlendRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator
			, const char* l_vtxShader
			, const char* l_fragShader
			, const char* l_spvPath
			, const char* l_rendererName
			, uint32_t l_mipLevelTtoRenderTo);



		void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex) override;

		void UpdateBuffers(const uint32_t l_currentSwapchainIndex,
			const VulkanEngine::CameraStructure& l_cameraStructure) override;

		void UpdateDescriptorSets() override;



		~UpsampleBlendRenderer();

	private:


		std::vector<VulkanTexture*> m_mipMapInputOutputImages;
		std::vector<VkFramebuffer> m_newFramebuffers;
		std::vector<VkImageView> m_framebufferImageViews{};
		std::vector<VkImageView> m_descriptorImageViews{};
		std::vector<glm::vec2> m_mipchainDimensions{};
		const uint32_t m_totalNumMipLevels{ 6 };
		const uint32_t m_mipLevelToRenderTo;

		VulkanBuffer* m_uniformBufferGpu;
	};


}