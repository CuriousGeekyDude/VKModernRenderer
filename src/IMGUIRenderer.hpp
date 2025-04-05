#pragma once



#include "Renderbase.hpp"


namespace ImGui
{
	struct ImGuiIO;
}

namespace RenderCore
{
	class IMGUIRenderer : public Renderbase
	{

	public:

		IMGUIRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator);

		void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex) override;


		void UpdateBuffers(const uint32_t l_currentSwapchainIndex,
			const VulkanEngine::CameraStructure& l_cameraStructure) override;


		void UpdateDescriptorSets() override;

		
		~IMGUIRenderer();

	private:


		std::vector<VulkanTexture*> m_swapchains;
		VkDescriptorPool m_imguiPool = VK_NULL_HANDLE;
		ImGui::ImGuiIO* m_io;

	};
}