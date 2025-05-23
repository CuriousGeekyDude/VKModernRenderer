#pragma once



#include "Renderbase.hpp"
#include "glm/glm.hpp"
#include <array>


namespace RenderCore
{


	class TiledDeferredLightningRenderer : public Renderbase
	{

		static constexpr uint32_t m_totalNumLights{ 232 };

		struct Light
		{
			glm::vec4 m_positionAndRadius;
		};



		struct UniformBuffer
		{
			glm::mat4   m_inMtx;
			glm::mat4   m_viewMatrix;
			glm::mat4   m_invProjMatrix;
			glm::mat4	m_projMatrix;
			glm::vec4   m_cameraPos;
		};

	public:
		TiledDeferredLightningRenderer
		(VulkanEngine::VulkanRenderContext& l_vkContextCreator
			, const char* l_computeShader
			, const char* l_spvPath);
		void SetSwitchToDebugTiled(bool l_switch);



	protected:

		void InitializePositionData
		(std::array<glm::vec4, m_totalNumLights>& l_positionData);

		void InitializeLightBuffer
		(std::array<Light, m_totalNumLights>& l_lightBuffer
			, const std::array<glm::vec4, m_totalNumLights>& l_positionData);

		void InitializeVertexBuffer
		(const std::array<glm::vec4, m_totalNumLights>& l_positionData
			, std::array<glm::vec4, 8 * m_totalNumLights>& l_vertexBuffer
			, const std::array<glm::vec4, 8>& l_unitCube);

		void UpdateDescriptorSets() override;

		void UpdateBuffers(const uint32_t l_currentSwapchainIndex,
			const VulkanEngine::CameraStructure& l_cameraStructure) override;

		void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex) override;



	private:

		uint32_t m_uniformBufferGpuHandle;
		uint32_t m_lightBufferGpuHandle;
		uint32_t m_vertexBufferGpuHandle;
		uint32_t m_indicesBufferGpuHandle;
		uint32_t m_depthMapLightGpuHandle;
		std::vector<VulkanTexture*> m_colorOutputTextures;
		VulkanBuffer* m_debugBuffer;
		VkPipeline m_debugComputePipeline;
		bool m_switchToDebug{ false };
	};


}