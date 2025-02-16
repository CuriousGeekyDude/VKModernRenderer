#pragma once



#include "Renderbase.hpp"
#include "glm/glm.hpp"
#include <array>


namespace RenderCore
{


	class DeferredLightningRenderer : public Renderbase
	{

		static constexpr uint32_t m_totalNumLights{ 16 };
	
		struct Light
		{
			glm::vec4 m_position;
			glm::vec4 m_color{ 1.f, 1.f, 1.f, 1.f };

			float m_linear{ 0.7f };
			float m_quadratic{ 1.8f };
			float m_radius{ 5.090127 };
			float m_pad{ -1.f };
		};



		struct UniformBuffer
		{
			glm::mat4   m_inMtx;
			glm::mat4   m_viewMatrix;
			glm::vec4	m_cameraPos;
		};

	public:
		DeferredLightningRenderer
		(VulkanEngine::VulkanRenderContext& l_vkContextCreator
		,const char* l_vtxShader
		,const char* l_fragShader
		,const char* l_spvPath);


	protected:

		void InitializePositionData
		(std::array<glm::vec4, m_totalNumLights>& l_positionData);

		void InitializeLightBuffer
		(std::array<Light, m_totalNumLights>& l_lightBuffer
		, const std::array<glm::vec4, m_totalNumLights>& l_positionData);

		void InitializeVertexBuffer
		(const std::array<glm::vec4, m_totalNumLights>& l_positionData
		, std::array<glm::vec4, 8*m_totalNumLights>& l_vertexBuffer
		,const std::array<glm::vec4, 8>& l_unitCube);

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
	};


}