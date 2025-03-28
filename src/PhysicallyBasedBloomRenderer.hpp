#pragma once



#include "Renderbase.hpp"



namespace RenderCore
{
	class PhysicallyBasedBloomRenderer : public Renderbase
	{

		struct UniformBuffer
		{
			glm::vec4 m_mipchainDimensions{};
			uint32_t m_indexMipchain{};
			uint32_t m_pad0{};
			uint32_t m_pad1{};
			uint32_t m_pad2{};
		};

	
	public:

		PhysicallyBasedBloomRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator
			, const char* l_vtxShader
			, const char* l_fragShader
			, const char* l_spvPath);



		void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex) override;

		void UpdateBuffers(const uint32_t l_currentSwapchainIndex,
			const VulkanEngine::CameraStructure& l_cameraStructure) override;

		void UpdateDescriptorSets() override;


	private:


		std::vector<VulkanTexture*> m_mipMapInputOutputImages;
		std::vector<VkFramebuffer> m_newFramebuffers;
		std::vector<VkImageView> m_framebufferImageViews{};
		std::vector<glm::vec2> m_mipchainDimensions{};
		std::vector<uint32_t> m_currentMipchainIndexToSample{};
		const uint32_t m_totalNumMipLevels{6};
		
		VulkanBuffer* m_uniformBufferGpu;

	};
}