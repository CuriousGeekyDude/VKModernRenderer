#pragma once



#include "Renderbase.hpp"
#include <GLFW/glfw3.h>
#include "FrameGraph.hpp"

struct ImGuiIO;

namespace RenderCore
{
	class IMGUIRenderer : public Renderbase
	{

	public:

		IMGUIRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator, GLFWwindow* l_window);

		void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex) override;


		void UpdateBuffers(const uint32_t l_currentSwapchainIndex,
			const VulkanEngine::CameraStructure& l_cameraStructure) override;


		void UpdateDescriptorSets() override;

		void UpdateIncomingDataFromNodes();

		
		~IMGUIRenderer();

	private:


		std::vector<VulkanTexture*> m_swapchains;
		VkDescriptorPool m_imguiPool = VK_NULL_HANDLE;
		ImGuiIO* m_io;

		
		VulkanEngine::FrameGraphNode* m_indirectRenderer;
		uint32_t m_totalNumVisibleMeshes{};
	};
}