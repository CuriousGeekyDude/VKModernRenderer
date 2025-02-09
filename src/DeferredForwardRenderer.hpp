#pragma once


#include "Renderbase.hpp"

namespace RenderCore
{

	class DeferredForwardRenderer : public Renderbase
	{

	public:

		DeferredForwardRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator,
			const char* l_vertexShaderPath, const char* l_fragmentShaderPath,
			const char* l_spirvPath);


		virtual void UpdateBuffers(const uint32_t l_currentSwapchainIndex,
			const VulkanEngine::CameraStructure& l_cameraStructure) override;


		virtual void FillCommandBuffer(
			VkCommandBuffer l_commandBuffer,
			uint32_t l_currentSwapchainIndex) override;

	protected:

		virtual void UpdateDescriptorSets() override;

	private:

		std::vector<uint32_t> m_samplerTexturesHandles;
	};

}