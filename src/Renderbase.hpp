#pragma once 


#include "SpirvPipelineGenerator.hpp"
#include "VulkanRenderContext.hpp"
#include <vector>




namespace VulkanEngine
{
	struct CameraStructure;
}



namespace RenderCore

{

	class Renderbase
	{
	public:

		Renderbase(VulkanEngine::VulkanRenderContext& l_vkContextCreator);

		virtual void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex) = 0;


		//Updating storage buffers associated to the renderer
		virtual void UpdateStorageBuffers(uint32_t l_currentSwapchainIndex) {}

		

		void InitializeGraphicsPipeline(const std::vector<const char*>& l_shaders, const RenderCore::VulkanResourceManager::PipelineInfo& l_pInfo,
			uint32_t l_vtxConstSize = 0,
			uint32_t l_fragConstSize = 0);



		virtual void UpdateBuffers(const uint32_t l_currentSwapchainIndex, 
			const VulkanEngine::CameraStructure& l_cameraStructure) = 0;

		VkDescriptorPool& GetPool();

		//RenderCore::VulkanResourceManager::RenderPass GetRenderPass() const;

		void BeginRenderPass(VkRenderPass l_rp, VkFramebuffer l_fb, 
			VkCommandBuffer l_commandBuffer, size_t l_currentImage, size_t l_totalNumClearValues);

		static void InitDescriptorPoolForAllRenderers(VkDescriptorPool* l_pool,
			VulkanEngine::VulkanRenderContext& l_renderContext);

		void SetRenderPassAndFrameBuffer(const std::string& l_rendererName);


	protected:

		void SetNodeToAppropriateRenderpass(const std::string& l_renderpassName,
			Renderbase* l_renderpass);


		void GeneratePipelineFromSpirvBinaries(
			const std::string& l_spirvFilePath);


		//virtual void CreateRenderPass() = 0;
		virtual void UpdateDescriptorSets() = 0;
		//virtual void CreateFramebuffers() = 0;

		VulkanEngine::VulkanRenderContext& m_vulkanRenderContext;

		VkRenderPass m_renderPass = VK_NULL_HANDLE;
		VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;
		VkPipeline m_computePipeline = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
		static VkDescriptorPool m_descriptorPool;
		std::vector<VkDescriptorSet> m_descriptorSets;
		std::vector<uint32_t> m_framebufferHandles;
		std::vector<VulkanBuffer> m_uniformBuffers{};

	};

}