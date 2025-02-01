#pragma once






#include "Renderbase.hpp"
#include <memory>

namespace VulkanEngine
{
	struct CameraStructure;
}

namespace RenderCore
{

	class DeferredComputeRenderer : public Renderbase
	{
		//Should not exceed 256 since the tile sizes are 8x8 pixels
		static const uint32_t m_totalNumLights{ 64 };

		static const uint32_t m_totalNumBins{ 32 };

		struct SortedLight
		{
			uint32_t m_lightIndex;
			glm::vec4 m_viewPos;
			glm::vec4 m_viewAABBMin;
			glm::vec4 m_viewAABBMax;
			glm::vec4 m_worldPos;

			float m_linearizedViewPosZ;
			float m_linearizedViewAABBMinZ;
			float m_linearizedViewAABBMaxZ;
			float m_radius{};
		};

		struct Light
		{
			glm::vec4 m_position;
			glm::vec4 m_color{ 1.f, 1.f, 1.f, 1.f };

			float m_linear{ 0.7f };
			float m_quadratic{ 1.8f };
			float m_radius{5.090127};
			float m_pad{ -1.f };
		};


		struct DeferredUniformBufferCamera
		{
			glm::mat4 m_mvp;
			glm::vec4 m_viewMatrix;
			glm::vec4 m_cameraPos;


			float scale;
			float bias;
			float zNear;
			float zFar;
			float radius;
			float attScale;
			float distScale;
			uint32_t m_enableDeferred;
		};


		struct alignas(16) Uint32Aligned16
		{
			uint32_t m_value{0};
		};

		struct UniformBuffer
		{
			Light m_lights[m_totalNumLights];
			Uint32Aligned16 m_sortedLightsIndices[m_totalNumLights];

			
			Uint32Aligned16 m_bins[m_totalNumBins];

		};

	public:
		DeferredComputeRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator,
			const char* l_computeShaderFilePath,
			const std::string& l_spirvFile);


		virtual void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex) override;


		virtual void UpdateUniformBuffers(const uint32_t l_currentSwapchainIndex,
			const VulkanEngine::CameraStructure& l_cameraStructure) override;
	protected:

		void ApplyZBinning(const uint32_t l_currentSwapchainIndex
			 ,const VulkanEngine::CameraStructure& l_cameraStructure);

		//virtual void CreateFramebuffers() override;
		virtual void UpdateDescriptorSets() override;
		//virtual void CreateRenderPass() override;

	private:

		std::vector<UniformBuffer> m_uniformBuffersCPU;

		std::vector<std::vector<uint32_t>> m_tileBuffersCPU;

		std::vector<uint32_t> m_tileBuffersHandles;
		std::vector<uint32_t> m_uniformBuffersLightDataHandles;
		std::vector<uint32_t> m_uniformBuffersCameraHandles;
		std::vector<uint32_t> m_samplingTexturesHandles;
		std::vector<uint32_t> m_outputImageTexturesHandles;
		
	};


}