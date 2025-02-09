#pragma once


#include "Renderbase.hpp"
#include "CpuResourceServiceProvider.hpp"
#include <vector>
#include <array>
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


		struct DebugViewFrustum
		{
			std::array<glm::vec3, 8> m_debugViewFrustumCorners;
			std::array<glm::vec4, 6> m_debugViewFrustumPlanes;
			std::array<uint16_t, 24> m_indexBuffer;
			glm::mat4 m_viewMatrix;
			glm::mat4 m_projectionMatrix;
			
		};

	public:
		BoundingBoxWireframeRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator,
			const char* l_vertexShaderPath, const char* l_fragmentShaderPath,
			const char* l_spirvPath);



		void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex) override;

		void UpdateBuffers(const uint32_t l_currentSwapchainIndex,
			const VulkanEngine::CameraStructure& l_cameraStructure) override;

	private:

		void CreateBoundingBoxVerticesAndIndices(const VulkanEngine::CpuResource& l_boundingBoxData);

		void UpdateDescriptorSets() override;
		
		void ApplyDebugCPUFrustumCulling();



	private:

		std::vector<float> m_boundingBoxVertices{};
		std::vector<uint16_t> m_boundingBoxIndices{};
		uint32_t m_vertexBufferGpuHandle;
		uint32_t m_indexBufferGpuHandle;
		uint32_t m_debugViewFrustumIndexGpuHandle;
		uint32_t m_debugViewFrustumVertexGpuHandle;
		uint32_t m_colorIndicesOfWireframesGpuHandle;
		std::vector<uint32_t> m_colorIndicesOfWireframes;
		std::vector<uint32_t> m_uniformBufferHandles;
		std::vector<uint32_t> m_depthTextureHandles;
		std::vector<uint32_t> m_swapchainHandles;
		DebugViewFrustum m_debugViewFrustum;
		
	};

}