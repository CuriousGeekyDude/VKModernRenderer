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

		
		void UpdateSSAOUniform();

		void UpdateRadiusUpsamples();

		~IMGUIRenderer();

	private:


		std::vector<VulkanTexture*> m_swapchains;
		VkDescriptorPool m_imguiPool = VK_NULL_HANDLE;
		ImGuiIO* m_io;

		
		VulkanEngine::FrameGraphNode* m_indirectRenderer;
		uint32_t m_totalNumVisibleMeshes{};

		VulkanEngine::FrameGraphNode* m_ssaoRenderer;
		uint32_t m_ssaoSortedHandle{};
		float m_radiusSSAO{8.f};
		int m_offsetBufferSize{16};
		bool m_showSSAOTextureOnly{ false };
		bool m_cachedShowSSAOTextureOnly{ false };
		std::vector<VulkanTexture*> m_ssaoTextures;



		VulkanEngine::FrameGraphNode* m_upsampleBlendRenderer0;
		VulkanEngine::FrameGraphNode* m_upsampleBlendRenderer1;
		VulkanEngine::FrameGraphNode* m_upsampleBlendRenderer2;
		VulkanEngine::FrameGraphNode* m_upsampleBlendRenderer3;
		float m_upsampleRadius{ 0.005f };


		VulkanEngine::FrameGraphNode* m_fxxaRenderer;

	};
}