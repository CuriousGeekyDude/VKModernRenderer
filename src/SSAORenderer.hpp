#pragma once



//#include "CompositeRenderer.hpp"
//#include "QuadRenderer.hpp"
#include <optional>


namespace RenderCore
{
	class SSAORenderer : public CompositeRenderer
	{
	public:

		SSAORenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator,
			const RenderCore::VulkanResourceManager::RenderPass& l_renderpass,
			std::vector<VulkanTexture>& l_inputColorAttachment,
			std::vector<VulkanTexture>& l_depths,
			const std::vector<VkFramebuffer>& l_outputFrameBuffers);


		virtual void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex) override;



	private:

		VulkanTexture m_samplingTex;
		std::vector<VulkanTexture> m_ssaoTex, m_ssaoBlurXTex, m_ssaoBlurYTex;
		std::vector<VulkanTexture*> m_inputAndDepthAttachments;

		std::optional<QuadRenderer> m_ssaoQuadRenderer, m_ssaoBlurXQuadRenderer, m_ssaoBlurYQuadRenderer, m_ssaoFinalQuadRenderer;
	};
}