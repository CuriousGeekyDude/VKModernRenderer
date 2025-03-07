#pragma once




#include "Renderbase.hpp"



namespace RenderCore
{
	class DepthMapLightRenderer : public Renderbase
	{
		struct UniformBufferLight
		{
			glm::mat4 m_viewMatrix;
			glm::mat4 m_orthoMatrix;
			glm::vec4 m_pos;
		};

	public:

		DepthMapLightRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator
							 , const char* l_vtxShader, const char* l_fragShader
							 , const char* l_spvFile, const glm::vec4& l_lightPos);


		void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex) override;

		void UpdateBuffers(const uint32_t l_currentSwapchainIndex,
			const VulkanEngine::CameraStructure& l_cameraStructure) override;

		void UpdateDescriptorSets() override;


	private:
		UniformBufferLight m_uniformBufferCpu;
		uint32_t m_uniformBufferGpuHandle;
		std::vector<VulkanTexture*> m_depthMapGpuTextures;
		uint32_t m_indicesVerticesGpuBufferHandle;
		std::vector<VulkanBuffer*> m_instanceBuffersGpu;
		uint32_t m_indirectBufferGpuHandle;
	};
}