#pragma once



#include "Renderbase.hpp"


namespace RenderCore
{

	class WireframeDebugTiledDeferredRenderer:public Renderbase
	{


		struct Light
		{
			glm::vec4 m_min{ std::numeric_limits<float>::max() };
			glm::vec4 m_max{ std::numeric_limits<float>::min() };
			glm::mat4 m_worldTransform{};
			glm::vec4 m_color;
		};

		struct SortedLight
		{
			glm::vec4 m_minView{};
			glm::vec4 m_maxView{};
			glm::vec4 m_posView{};

			std::array<glm::vec4, 8> m_aabbView{};
		};


	public:
		WireframeDebugTiledDeferredRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator
			, const char* l_vtxShader, const char* l_fragShader, const char* l_spv);


		void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex) override;


		void UpdateBuffers(const uint32_t l_currentSwapchainIndex,
			const VulkanEngine::CameraStructure& l_cameraStructure) override;



		void UpdateDescriptorSets() override;


	private:

		std::vector<Light> m_lights;
		std::vector<SortedLight> m_sortedLights;
		std::vector<uint32_t> m_sortedIndicesOfLight;
		std::vector<uint32_t> m_bins;
		std::vector<uint8_t> m_lightAABBIntersectionTestFrustum;
		std::vector<uint32_t> m_grid;

		std::vector<glm::vec4> m_verticesBufferCpu;
		std::array<uint16_t, 8> m_indicesBufferCpu;


		std::vector<VulkanTexture*> m_swapchainTextures;
		std::vector<VulkanTexture*> m_depthTextures;

		VulkanBuffer* m_vertexBufferGpu;
		VulkanBuffer* m_indexBufferGpu;

		VulkanBuffer* m_vertexBufferGridVerticalGpu;
		VulkanBuffer* m_vertexBufferGridHorizontalGpu;
		VulkanBuffer* m_indexBufferGridGpu;


	};


}