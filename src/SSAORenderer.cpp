


#include "SSAORenderer.hpp"
#include <format>


namespace RenderCore
{
	SSAORenderer::SSAORenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator,
		const RenderCore::VulkanResourceManager::RenderPass& l_renderpass,
		std::vector<VulkanTexture>& l_inputColorAttachment,
		std::vector<VulkanTexture>& l_depths,
		const std::vector<VkFramebuffer>& l_outputFrameBuffers)
		:CompositeRenderer(l_vkContextCreator, l_renderpass),
		m_samplingTex(l_vkContextCreator.GetResourceManager().LoadTexture2D("data/rot_texture.bmp"))

	{
		auto lv_totalNumSwapChains = l_vkContextCreator.GetContextCreator().m_vkDev.m_swapchainImages.size();
		auto& lv_uniformBuffers = l_vkContextCreator.GetResourceManager().GetUniformBuffers();
		m_framebufferHandles = l_outputFrameBuffers;
		std::vector<const char*> lv_shaders{2};
		lv_shaders[0] = "data/shaders/chapter08/VK02_Quad.vert";


		std::vector<VkFramebuffer> lv_framebuffersOfSomeRenderer{lv_totalNumSwapChains};
		m_ssaoTex.resize(lv_totalNumSwapChains);
		m_ssaoBlurXTex.resize(lv_totalNumSwapChains);
		m_ssaoBlurYTex.resize(lv_totalNumSwapChains);


		for (size_t j = 0; j < lv_totalNumSwapChains; ++j) {
			m_inputAndDepthAttachments.push_back(&l_inputColorAttachment[j]);
			m_inputAndDepthAttachments.push_back(&l_depths[j]);
		}


		for (size_t i = 0; i < lv_totalNumSwapChains; ++i) {
			m_ssaoTex[i] = l_vkContextCreator.GetResourceManager()
				.CreateTextureForOffscreenFrameBuffer(std::format(" ssaoTex-SSAO {} ", i));
			m_ssaoBlurXTex[i] = l_vkContextCreator.GetResourceManager()
				.CreateTextureForOffscreenFrameBuffer(std::format(" ssaoBlurXTex-SSAO {} ", i));
			m_ssaoBlurYTex[i] = l_vkContextCreator.GetResourceManager()
				.CreateTextureForOffscreenFrameBuffer(std::format(" ssaoBlurYTex-SSAO {} ", i));
		}

		std::vector<VulkanResourceManager::BufferResourceShader> lv_uniformBufferShaderResources{};
		for (size_t i = 0; i < lv_totalNumSwapChains; ++i) {
			VulkanResourceManager::BufferResourceShader lv_uniformBufferShaderResource{};
			lv_uniformBufferShaderResource.m_buffer = lv_uniformBuffers[i];
			lv_uniformBufferShaderResource.m_descriptorInfo.m_shaderStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			lv_uniformBufferShaderResource.m_descriptorInfo.m_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			lv_uniformBufferShaderResource.m_offset = 0;
			lv_uniformBufferShaderResource.m_size = lv_uniformBuffers[0].size;

			lv_uniformBufferShaderResources.push_back(lv_uniformBufferShaderResource);
		}

		std::vector<VulkanResourceManager::DescriptorSetResources> lv_descriptorSetRes{lv_totalNumSwapChains};

		std::vector<VulkanResourceManager::TextureResourceShader> lv_textureShaderResources{2*lv_totalNumSwapChains};

		for (size_t i = 0; i < lv_totalNumSwapChains; ++i) {
			lv_textureShaderResources[2*i] = VulkanResourceManager::TextureResourceShader{
				.m_descriptorInfo = {.m_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .m_shaderStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
				.m_texture = l_depths[i]};
			lv_textureShaderResources[2 * i].m_texture.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_textureShaderResources[2*i+1] = VulkanResourceManager::TextureResourceShader{
				.m_descriptorInfo = {.m_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .m_shaderStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
				.m_texture = m_samplingTex };
		}

		for (size_t i = 0; i < lv_totalNumSwapChains; ++i) {
			lv_descriptorSetRes[i].m_buffers.push_back(lv_uniformBufferShaderResources[i]);
			lv_descriptorSetRes[i].m_textures.push_back(lv_textureShaderResources[2*i]);
			lv_descriptorSetRes[i].m_textures.push_back(lv_textureShaderResources[2*i+1]);

		}
		

		
		VulkanResourceManager::RenderPass lv_ssaoQuadRenderPass(l_vkContextCreator.GetContextCreator().m_vkDev, false, 
			RenderPassCreateInfo{.clearColor_ = true, .clearDepth_ = false, 
			.flags_ = eRenderPassBit_First_ColorAttach_ColorAttach});
		VulkanResourceManager::PipelineInfo lv_ssaoQuadPipelineInfo
		{.m_width = l_vkContextCreator.GetContextCreator().m_vkDev.m_framebufferWidth,
		.m_height = l_vkContextCreator.GetContextCreator().m_vkDev.m_framebufferHeight,
		.m_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST ,
		.m_useDepth = false, .m_useBlending = false, .m_dynamicScissorState = false};

		for (size_t i = 0; i < lv_totalNumSwapChains; ++i) {
			lv_framebuffersOfSomeRenderer[i] = l_vkContextCreator.GetResourceManager()
				.CreateFrameBuffer(lv_ssaoQuadRenderPass, std::vector<VulkanTexture>{m_ssaoTex[i]}
			, std::format(" ssaoTex-Framebuffer-SSAO {} ", i).c_str());
		}
		lv_shaders[1] = "data/shaders/chapter08/VK02_SSAO.frag";
		m_ssaoQuadRenderer.emplace(l_vkContextCreator, "SSAO ", lv_ssaoQuadRenderPass, lv_ssaoQuadPipelineInfo,
			lv_framebuffersOfSomeRenderer, &m_ssaoTex,
			lv_descriptorSetRes, lv_shaders);
		
		lv_textureShaderResources.clear();
		lv_descriptorSetRes.clear();
		lv_descriptorSetRes.resize(lv_totalNumSwapChains);
		lv_textureShaderResources.resize(lv_totalNumSwapChains);

		for (size_t i = 0; i < lv_totalNumSwapChains; ++i) {
			lv_textureShaderResources[i] = VulkanResourceManager::TextureResourceShader{ .m_descriptorInfo = 
				{.m_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .m_shaderStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
				.m_texture = m_ssaoTex[i]};
			lv_textureShaderResources[i].m_texture.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		for (size_t i = 0; i < lv_totalNumSwapChains; ++i) {
			lv_descriptorSetRes[i].m_buffers.push_back(lv_uniformBufferShaderResources[i]);
			lv_descriptorSetRes[i].m_textures.push_back(lv_textureShaderResources[i]);
		}

		VulkanResourceManager::RenderPass lv_blurXQuadRenderPass(l_vkContextCreator.GetContextCreator().m_vkDev, false,
			RenderPassCreateInfo{ .clearColor_ = true, .clearDepth_ = false, 
			.flags_ = eRenderPassBit_First_ColorAttach_ColorAttach });
		VulkanResourceManager::PipelineInfo lv_blurXQuadPipelineInfo{ .m_width 
			= l_vkContextCreator.GetContextCreator().m_vkDev.m_framebufferWidth,
		.m_height = l_vkContextCreator.GetContextCreator().m_vkDev.m_framebufferHeight,
		.m_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST ,
		.m_useDepth = false, .m_useBlending = false, .m_dynamicScissorState = false };


		for (uint32_t i = 0; i < lv_totalNumSwapChains; ++i) {
			lv_framebuffersOfSomeRenderer[i] = l_vkContextCreator.GetResourceManager()
				.CreateFrameBuffer(lv_blurXQuadRenderPass, std::vector<VulkanTexture>{m_ssaoBlurXTex[i]},
					std::format(" ssaoBlurXTex-Framebuffer-SSAO {} ", i).c_str());
		}

		lv_shaders[1] = "data/shaders/chapter08/VK02_SSAOBlurX.frag";
		m_ssaoBlurXQuadRenderer.emplace(l_vkContextCreator, "SSAO ", lv_blurXQuadRenderPass, lv_blurXQuadPipelineInfo,
			lv_framebuffersOfSomeRenderer, &m_ssaoBlurXTex, lv_descriptorSetRes, lv_shaders);

		

		lv_textureShaderResources.clear();
		lv_descriptorSetRes.clear();
		lv_descriptorSetRes.resize(lv_totalNumSwapChains);
		lv_textureShaderResources.resize(lv_totalNumSwapChains);



		for (size_t i = 0; i < lv_totalNumSwapChains; ++i) {
			lv_textureShaderResources[i] = VulkanResourceManager::TextureResourceShader{ .m_descriptorInfo =
				{.m_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .m_shaderStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
				.m_texture = m_ssaoBlurXTex[i] };
			lv_textureShaderResources[i].m_texture.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		for (size_t i = 0; i < lv_totalNumSwapChains; ++i) {
			lv_descriptorSetRes[i].m_buffers.push_back(lv_uniformBufferShaderResources[i]);
			lv_descriptorSetRes[i].m_textures.push_back(lv_textureShaderResources[i]);
		}

		VulkanResourceManager::RenderPass lv_blurYQuadRenderPass(l_vkContextCreator.GetContextCreator().m_vkDev, false,
			RenderPassCreateInfo{ .clearColor_ = true, .clearDepth_ = false, 
			.flags_ = eRenderPassBit_First_ColorAttach_ColorAttach });
		VulkanResourceManager::PipelineInfo lv_blurYQuadPipelineInfo{ .m_width 
			= l_vkContextCreator.GetContextCreator().m_vkDev.m_framebufferWidth,
		.m_height = l_vkContextCreator.GetContextCreator().m_vkDev.m_framebufferHeight,
		.m_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST ,
		.m_useDepth = false, .m_useBlending = false, .m_dynamicScissorState = false };


		for (uint32_t i = 0; i < lv_totalNumSwapChains; ++i) {
			lv_framebuffersOfSomeRenderer[i] = l_vkContextCreator.GetResourceManager()
				.CreateFrameBuffer(lv_blurXQuadRenderPass, std::vector<VulkanTexture>{m_ssaoBlurYTex[i]}
				,std::format(" ssaoBlurYTex-Framebuffer-SSAO {} ",i).c_str());
		}

		lv_shaders[1] = "data/shaders/chapter08/VK02_SSAOBlurY.frag";
		m_ssaoBlurYQuadRenderer.emplace(l_vkContextCreator, "SSAO ", lv_blurYQuadRenderPass, lv_blurYQuadPipelineInfo,
			lv_framebuffersOfSomeRenderer, &m_ssaoBlurYTex,lv_descriptorSetRes,lv_shaders);


		lv_textureShaderResources.clear();
		lv_descriptorSetRes.clear();
		lv_descriptorSetRes.resize(lv_totalNumSwapChains);
		lv_textureShaderResources.resize(2*lv_totalNumSwapChains);

		for (size_t i = 0, j = 0; i < lv_textureShaderResources.size() && j < lv_totalNumSwapChains; i += 2, ++j) {
			lv_textureShaderResources[i].m_descriptorInfo = { .m_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.m_shaderStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT };
			lv_textureShaderResources[i].m_texture = l_inputColorAttachment[j];
			lv_textureShaderResources[i].m_texture.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			lv_textureShaderResources[i + 1].m_descriptorInfo = { .m_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.m_shaderStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT };
			lv_textureShaderResources[i + 1].m_texture = m_ssaoBlurYTex[j];
			lv_textureShaderResources[i+1].m_texture.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		for (size_t i = 0; i < lv_totalNumSwapChains; ++i) {

			lv_descriptorSetRes[i].m_buffers.push_back(lv_uniformBufferShaderResources[i]);
			lv_descriptorSetRes[i].m_textures.push_back(lv_textureShaderResources[2*i]);
			lv_descriptorSetRes[i].m_textures.push_back(lv_textureShaderResources[2 * i+1]);
			
		}

	
		VulkanResourceManager::PipelineInfo lv_ssaoFinalQuadPipelineInfo{ 
			.m_width = l_vkContextCreator.GetContextCreator().m_vkDev.m_framebufferWidth,
		.m_height = l_vkContextCreator.GetContextCreator().m_vkDev.m_framebufferHeight,
		.m_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST ,
		.m_useDepth = false, .m_useBlending = false, .m_dynamicScissorState = false };


		lv_shaders[1] = "data/shaders/chapter08/VK02_SSAOFinal.frag";
		m_ssaoFinalQuadRenderer.emplace(l_vkContextCreator, "SSAO ", m_vulkanRenderContext.GetOffscreenRenderPassNoDepth(), lv_ssaoFinalQuadPipelineInfo,
			m_vulkanRenderContext.GetSwapchainFramebufferNoDepth(), nullptr, lv_descriptorSetRes, lv_shaders);


		m_renderers.emplace_back(m_ssaoQuadRenderer.value(), true, false);
		m_renderers.emplace_back(m_ssaoBlurXQuadRenderer.value(), true, false);
		m_renderers.emplace_back(m_ssaoBlurYQuadRenderer.value(), true, false);
		m_renderers.emplace_back(m_ssaoFinalQuadRenderer.value(), true, false);


	}

	void SSAORenderer::FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
		uint32_t l_currentSwapchainIndex)
	{

		transitionImageLayoutCmd(l_cmdBuffer, m_inputAndDepthAttachments[2 * l_currentSwapchainIndex]->image.image,
			m_inputAndDepthAttachments[2 * l_currentSwapchainIndex]->format,
			m_inputAndDepthAttachments[2 * l_currentSwapchainIndex]->Layout,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_inputAndDepthAttachments[2 * l_currentSwapchainIndex]->Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		transitionImageLayoutCmd(l_cmdBuffer, m_inputAndDepthAttachments[2 * l_currentSwapchainIndex + 1]->image.image,
			m_inputAndDepthAttachments[2 * l_currentSwapchainIndex + 1]->format,
			m_inputAndDepthAttachments[2 * l_currentSwapchainIndex + 1]->Layout,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_inputAndDepthAttachments[2 * l_currentSwapchainIndex + 1]->Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;



		CompositeRenderer::FillCommandBuffer(l_cmdBuffer, l_currentSwapchainIndex);



		transitionImageLayoutCmd(l_cmdBuffer, m_inputAndDepthAttachments[2*l_currentSwapchainIndex]->image.image,
			m_inputAndDepthAttachments[2 * l_currentSwapchainIndex]->format,
			m_inputAndDepthAttachments[2 * l_currentSwapchainIndex]->Layout,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		m_inputAndDepthAttachments[2 * l_currentSwapchainIndex]->Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		
		transitionImageLayoutCmd(l_cmdBuffer, m_inputAndDepthAttachments[2 * l_currentSwapchainIndex + 1]->image.image,
			m_inputAndDepthAttachments[2 * l_currentSwapchainIndex + 1]->format,
			m_inputAndDepthAttachments[2 * l_currentSwapchainIndex + 1]->Layout,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		m_inputAndDepthAttachments[2 * l_currentSwapchainIndex+1]->Layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	}
}