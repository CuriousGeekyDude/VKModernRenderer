#pragma once





#include "Renderbase.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace RenderCore
{

	class SingleModelRenderer : public Renderbase
	{

		struct UniformBuffer
		{
			glm::mat4 m_viewMatrix;
			glm::mat4 m_projectionMatrix;
			glm::vec4 m_cameraPos;

		};

	public:

		SingleModelRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator);


		void Init(const std::vector<glm::vec3>& l_vertices, const std::vector<uint32_t> l_indices
			, const char* l_vtxShader, const char* l_fragShader, const char* l_spv);


		void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex) override;

		void UpdateBuffers(const uint32_t l_currentSwapchainIndex,
			const VulkanEngine::CameraStructure& l_cameraStructure) override;

		void UpdateDescriptorSets() override;

	private:
		uint32_t m_uniformBufferGpuHandle;
		uint32_t m_vertexBufferGpuHandle;
		uint32_t m_indexBufferGpuHandle;
		uint32_t m_indexCount;
		std::vector<VulkanTexture*> m_swapchainTextures;
		std::vector<VulkanTexture*> m_depthTextures;
	};

	
}
