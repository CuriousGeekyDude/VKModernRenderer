#pragma once


#include "Renderbase.hpp"
#include "CpuResourceServiceProvider.hpp"
#include <vector>
#include <glm/glm.hpp>


namespace VulkanEngine
{
	struct CameraStructure;
}

namespace RenderCore
{

	class BoundingBoxWireframeRenderer : public RenderCore::Renderbase
	{

		struct UniformBuffer
		{
			glm::mat4   m_inMtx;
			glm::mat4   m_viewMatrix;
			glm::vec4	m_cameraPos;
		};

	public:
		BoundingBoxWireframeRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator,
			const char* l_vertexShaderPath, const char* l_fragmentShaderPath,
			const char* l_spirvPath);



		void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex) override;

		void UpdateUniformBuffers(const uint32_t l_currentSwapchainIndex,
			const VulkanEngine::CameraStructure& l_cameraStructure) override;

	private:

		void CreateBoundingBoxVerticesAndIndices(const VulkanEngine::CpuResource& l_boundingBoxData);

		void UpdateDescriptorSets() override;
		




	private:

		std::vector<float> m_boundingBoxVertices{};
		std::vector<uint16_t> m_boundingBoxIndices{};
		uint32_t m_vertexBufferGpuHandle;
		uint32_t m_indexBufferGpuHandle;
		std::vector<uint32_t> m_uniformBufferHandles;
		std::vector<uint32_t> m_depthTextureHandles;
		std::vector<uint32_t> m_swapchainHandles;
	};

}