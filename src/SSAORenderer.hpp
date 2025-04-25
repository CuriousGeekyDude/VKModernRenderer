#pragma once




#include "Renderbase.hpp"

namespace RenderCore
{

	class SSAORenderer : public Renderbase
	{

	public:

		struct UniformBufferMatrices
		{
			glm::mat4   m_viewMatrix;
			glm::mat4   m_projectionMatrix;

			float m_radius;
			int m_offsetBufferSize;
			float m_padding0;
			float m_padding1;
		};

		struct UniformBufferSSAOOffsets
		{
			glm::vec4 m_offsets[64];
		};

	public:

		SSAORenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator
					, const char* l_vtxShader
					, const char* l_fragShader
					, const char* l_spvPath);



		void UpdateDescriptorSets() override;

		void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex) override;

		void UpdateBuffers(const uint32_t l_currentSwapchainIndex,
			const VulkanEngine::CameraStructure& l_cameraStructure) override;

		void SetUniformBuffer(const UniformBufferMatrices& l_newUniform);


		

	private:


		uint32_t m_gpuOffsetsHandle;
		uint32_t m_gpuRandomRotationsTextureHandle;
		uint32_t m_gpuUniformBufferHandle;


		UniformBufferMatrices m_uniformCpu;

	};

}