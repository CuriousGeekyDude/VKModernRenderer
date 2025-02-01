


#include "Renderbase.hpp"
#include "ErrorCheck.hpp"
#include <format>

namespace RenderCore
{
	VkDescriptorPool Renderbase::m_descriptorPool = VK_NULL_HANDLE;

	Renderbase::Renderbase(VulkanEngine::VulkanRenderContext& ctx_) : 
		m_vulkanRenderContext(ctx_)
	{
		if (VK_NULL_HANDLE == m_descriptorPool) {
			InitDescriptorPoolForAllRenderers(&m_descriptorPool, m_vulkanRenderContext);
		}

	}

	VkDescriptorPool& Renderbase::GetPool()
	{
		return m_descriptorPool;
	}



	void Renderbase::SetRenderPassAndFrameBuffer(const std::string& l_rendererName)
	{
		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();

		auto* lv_node = lv_frameGraph.RetrieveNode(l_rendererName);

		m_framebufferHandles.resize(lv_totalNumSwapchains);

		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
			m_framebufferHandles[i] = lv_node->m_frameBufferHandles[i];
		}

		m_renderPass = lv_node->m_renderpass;

	}


	void Renderbase::GeneratePipelineFromSpirvBinaries(
		const std::string& l_spirvFilePath)
	{
		using namespace ErrorCheck;
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_totalNumSwapChains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();

		VulkanEngine::SpirvPipelineGenerator lv_pipelineGenerator(l_spirvFilePath);
		const auto& lv_vkSetLayoutDatas = lv_pipelineGenerator.GenerateDescriptorSetLayouts();


		VULKAN_CHECK(vkCreateDescriptorSetLayout(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device,
			&lv_vkSetLayoutDatas[0].m_setLayoutCreateInfo, nullptr, &m_descriptorSetLayout));
		lv_vkResManager.AddVulkanDescriptorSetLayout(m_descriptorSetLayout);


		for (size_t i = 0; i < lv_totalNumSwapChains; ++i) {
			m_descriptorSets.push_back(lv_vkResManager.CreateDescriptorSet(m_descriptorPool, m_descriptorSetLayout,
				std::format(" descriptorSet {} ", i).c_str()));
		}


		m_pipelineLayout = lv_vkResManager.CreatePipelineLayout(m_descriptorSetLayout, " pipelineLayout ");


	}


	void Renderbase::InitDescriptorPoolForAllRenderers(VkDescriptorPool* l_pool,
		VulkanEngine::VulkanRenderContext& l_renderContext)
	{
		using namespace ErrorCheck;
		auto& lv_vulkanResourceManager = l_renderContext.GetResourceManager();
		auto lv_totalNumSwapchain = l_renderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();

		std::array<VkDescriptorPoolSize, 3> lv_poolSizes{};
		lv_poolSizes[0].descriptorCount = lv_totalNumSwapchain * 64;
		lv_poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		lv_poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		lv_poolSizes[1].descriptorCount = lv_totalNumSwapchain * 64;

		lv_poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		lv_poolSizes[2].descriptorCount = lv_totalNumSwapchain * 256;

		VkDescriptorPoolCreateInfo lv_poolCreateInfo{};
		lv_poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		lv_poolCreateInfo.maxSets = 32 * lv_totalNumSwapchain;
		lv_poolCreateInfo.pNext = nullptr;
		lv_poolCreateInfo.poolSizeCount = (uint32_t)lv_poolSizes.size();
		lv_poolCreateInfo.pPoolSizes = lv_poolSizes.data();
		lv_poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
			| VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;


		VULKAN_CHECK(vkCreateDescriptorPool(l_renderContext.GetContextCreator().m_vkDev.m_device, &lv_poolCreateInfo,
			nullptr, l_pool));

		lv_vulkanResourceManager.AddVulkanDescriptorPool(m_descriptorPool);
	}

	void Renderbase::InitializeGraphicsPipeline(const std::vector<const char*>& l_shaders, const RenderCore::VulkanResourceManager::PipelineInfo& l_pInfo,
		uint32_t l_vtxConstSize,
		uint32_t l_fragConstSize)
	{
		using namespace ErrorCheck;

		m_pipelineLayout = m_vulkanRenderContext.GetResourceManager().
			CreatePipelineLayoutWithPush(m_descriptorSetLayout," Pipeline-Layout-Renderbase ",
				l_vtxConstSize, l_fragConstSize);

		m_graphicsPipeline = m_vulkanRenderContext.GetResourceManager().CreateGraphicsPipeline(m_renderPass,
			m_pipelineLayout, l_shaders, " Graphics-Pipeline-RenderBase " ,l_pInfo);
	}



	//RenderCore::VulkanResourceManager::RenderPass Renderbase::GetRenderPass() const { return m_renderPass; }


	/*RenderCore::VulkanResourceManager::PipelineInfo Renderbase::InitializeRenderPassAndFramebuffer(const RenderCore::VulkanResourceManager::PipelineInfo& l_pInfo,
		const std::vector<VulkanTexture>& l_outputs,
		const RenderCore::VulkanResourceManager::RenderPass& l_renderPass,
		const RenderCore::VulkanResourceManager::RenderPass& l_fallbackPass)
	{
		RenderCore::VulkanResourceManager::PipelineInfo lv_newPipelineInfo = l_pInfo;

		if (false == l_outputs.empty()) {

			m_renderPass = (VK_NULL_HANDLE != l_renderPass.m_renderpass) ? l_renderPass :
				(true == isDepthFormat(l_outputs[0].format) && 1 == l_outputs.size()) ?
				m_vulkanRenderContext.GetResourceManager().CreateDepthOnlyRenderPass(l_outputs) :
				(2 == l_outputs.size()) ? m_vulkanRenderContext.GetOffscreenRenderPassDepth() :
				m_vulkanRenderContext.GetOffscreenRenderPassNoDepth();

			for (uint32_t i = 0; i < m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size(); ++i) {
				m_framebuffers.push_back(m_vulkanRenderContext.GetResourceManager().CreateFrameBuffer(m_renderPass, l_outputs));
			}


			lv_newPipelineInfo.m_height = l_outputs[0].height;
			lv_newPipelineInfo.m_width = l_outputs[0].width;
		}
		else {
			m_renderPass = (VK_NULL_HANDLE != l_renderPass.m_renderpass) ? l_renderPass : l_fallbackPass;
		}

		return lv_newPipelineInfo;
	}*/

	void Renderbase::BeginRenderPass(VkRenderPass l_rp, VkFramebuffer l_fb,
		VkCommandBuffer l_commandBuffer, size_t l_currentImage, size_t l_totalNumClearValues)
	{
		std::vector<VkClearValue> lv_clearValues;
		lv_clearValues.resize(l_totalNumClearValues);

		if (1 == l_totalNumClearValues) 
		{
			lv_clearValues[0] = VkClearValue{ .color = { 1.0f, 1.0f, 1.0f, 1.0f } };
		}
		else {
			for (size_t i = 0; i < l_totalNumClearValues - 1; ++i) {
				lv_clearValues[i] = VkClearValue{ .color = { 1.0f, 1.0f, 1.0f, 1.0f } };
			}
			lv_clearValues[l_totalNumClearValues - 1] = VkClearValue{ .depthStencil = { 1.0f, 0 } };
		}


		const VkRect2D rect{
			.offset = { 0, 0 },
			.extent = {.width = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferWidth, 
			.height = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferHeight}
		};

		m_vulkanRenderContext.BeginRenderPass(l_commandBuffer, l_rp, l_currentImage, rect,
			l_fb,
			l_totalNumClearValues,
			lv_clearValues.data());

		vkCmdBindPipeline(l_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
		vkCmdBindDescriptorSets(l_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, 
			&m_descriptorSets[l_currentImage], 0, nullptr);
	}
}