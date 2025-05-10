



#include "IMGUIRenderer.hpp"
#include "IndirectRenderer.hpp"
#include "SSAORenderer.hpp"
#include "PresentSwapchainRenderer.hpp"
#include "UpsampleBlendRenderer.hpp"
#include "TiledDeferredLightningRenderer.hpp"
#include "DeferredLightningRenderer.hpp"
#include "SingleModelRenderer.hpp"
#include "DepthMapLightRenderer.hpp"

#include "imgui_impl_glfw.h"
#define IMGUI_IMPL_VULKAN_USE_VOLK
#include "imgui_impl_vulkan.h"
#include <array>

namespace RenderCore
{


	IMGUIRenderer::IMGUIRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator, GLFWwindow* l_window)
		:Renderbase(l_vkContextCreator)
	{

		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();


		m_ssaoTextures.resize(lv_totalNumSwapchains);
		m_swapchains.resize(lv_totalNumSwapchains);
		m_tiledDeferredOutput.resize(lv_totalNumSwapchains);
		m_fxaaInput.resize(lv_totalNumSwapchains);



		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
			m_swapchains[i] = &lv_vkResManager.RetrieveGpuTexture("Swapchain", i);
			m_ssaoTextures[i] = &lv_vkResManager.RetrieveGpuTexture("OcclusionFactor", i);
			m_tiledDeferredOutput[i] = &lv_vkResManager.RetrieveGpuTexture("DeferredLightningColorTexture", i);
			m_fxaaInput[i] = &lv_vkResManager.RetrieveGpuTexture("BlurSceneLinearInterpolated", i);
			
		}


		std::array<VkDescriptorPoolSize, 2> lv_poolSizes;
		
		lv_poolSizes[0].descriptorCount = 512;
		lv_poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		lv_poolSizes[0].descriptorCount = 64;
		lv_poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		VkDescriptorPoolCreateInfo lv_poolCreateInfo{};
		lv_poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		lv_poolCreateInfo.maxSets = 128;
		lv_poolCreateInfo.pNext = nullptr;
		lv_poolCreateInfo.poolSizeCount = (uint32_t)lv_poolSizes.size();
		lv_poolCreateInfo.pPoolSizes = lv_poolSizes.data();
		lv_poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
			| VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;


		CheckVkResult(vkCreateDescriptorPool(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device, &lv_poolCreateInfo,
			nullptr, &m_imguiPool));

		lv_vkResManager.AddVulkanDescriptorPool(m_imguiPool);


		SetRenderPassAndFrameBuffer("IMGUI");
		SetNodeToAppropriateRenderpass("IMGUI", this);
		lv_frameGraph.IncrementNumNodesPerCmdBuffer(2);


		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		m_io = &ImGui::GetIO();
		m_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

		ImGui_ImplGlfw_InitForVulkan( l_window,true);
		ImGui_ImplVulkan_InitInfo lv_imguiVulkanInit;
		memset(&lv_imguiVulkanInit, 0, sizeof(ImGui_ImplVulkan_InitInfo));
		lv_imguiVulkanInit.ApiVersion = VK_API_VERSION_1_3;
		lv_imguiVulkanInit.Allocator = nullptr;
		lv_imguiVulkanInit.CheckVkResultFn = &CheckVkResult;
		lv_imguiVulkanInit.DescriptorPool = m_imguiPool;
		lv_imguiVulkanInit.Device = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device;
		lv_imguiVulkanInit.ImageCount = (uint32_t)lv_totalNumSwapchains;
		lv_imguiVulkanInit.MinImageCount = 2;
		lv_imguiVulkanInit.MinAllocationSize = 1024 * 1024;
		lv_imguiVulkanInit.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		lv_imguiVulkanInit.PhysicalDevice = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_physicalDevice;
		lv_imguiVulkanInit.PipelineCache = VK_NULL_HANDLE;
		lv_imguiVulkanInit.Queue = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_mainQueue1;
		lv_imguiVulkanInit.QueueFamily = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_mainFamily;
		lv_imguiVulkanInit.RenderPass = m_renderPass;
		lv_imguiVulkanInit.Subpass = 0;
		lv_imguiVulkanInit.UseDynamicRendering = false;
		lv_imguiVulkanInit.Instance = m_vulkanRenderContext.GetContextCreator().m_vulkanInstance.instance;
		


		ImGui_ImplVulkan_Init(&lv_imguiVulkanInit);


		m_indirectRenderer = lv_frameGraph.RetrieveNode("IndirectGbuffer");
		m_ssaoRenderer = lv_frameGraph.RetrieveNode("SSAO");
		m_tiledDeferredLightningRenderer = lv_frameGraph.RetrieveNode("TiledDeferredLightning");
		m_deferredLightningRenderer = lv_frameGraph.RetrieveNode("DeferredLightning");
		m_fxxaRenderer = lv_frameGraph.RetrieveNode("FXAA");
		m_upsampleBlendRenderer0 = lv_frameGraph.RetrieveNode("UpsampleBlend4");
		m_upsampleBlendRenderer1 = lv_frameGraph.RetrieveNode("UpsampleBlend3");
		m_upsampleBlendRenderer2 = lv_frameGraph.RetrieveNode("UpsampleBlend2");
		m_upsampleBlendRenderer3 = lv_frameGraph.RetrieveNode("UpsampleBlend1");
		m_pointLightCubeRenderer = lv_frameGraph.RetrieveNode("PointLightCube");

		m_omniDirectional0 = lv_frameGraph.RetrieveNode("DepthMapOmnidirectionalPointLight0");
		m_omniDirectional1 = lv_frameGraph.RetrieveNode("DepthMapOmnidirectionalPointLight1");
		m_omniDirectional2 = lv_frameGraph.RetrieveNode("DepthMapOmnidirectionalPointLight2");
		m_omniDirectional3 = lv_frameGraph.RetrieveNode("DepthMapOmnidirectionalPointLight3");
		m_omniDirectional4 = lv_frameGraph.RetrieveNode("DepthMapOmnidirectionalPointLight4");
		m_omniDirectional5 = lv_frameGraph.RetrieveNode("DepthMapOmnidirectionalPointLight5");




		m_ssaoSortedHandle = lv_frameGraph.FindSortedHandleFromGivenNodeName("SSAO");
		m_tiledDeferredSortedHandle = lv_frameGraph.FindSortedHandleFromGivenNodeName("TiledDeferredLightning");
		assert(std::numeric_limits<uint32_t>::max() != m_ssaoSortedHandle);
		assert(std::numeric_limits<uint32_t>::max() != m_tiledDeferredSortedHandle);

	}


	void IMGUIRenderer::UpdateIncomingDataFromNodes()
	{
		//Data from IndirectGbuffer renderer
		{
			IndirectRenderer* lv_indirect = (IndirectRenderer*)m_indirectRenderer->m_renderer;
			m_totalNumVisibleMeshes = lv_indirect->GetTotalNumVisibleMeshes();
			
		}
	}

	void IMGUIRenderer::UpdateRadiusUpsamples()
	{
		UpsampleBlendRenderer* lv_upsampleRenderer0 = (UpsampleBlendRenderer*)m_upsampleBlendRenderer0->m_renderer;
		UpsampleBlendRenderer* lv_upsampleRenderer1 = (UpsampleBlendRenderer*)m_upsampleBlendRenderer1->m_renderer;
		UpsampleBlendRenderer* lv_upsampleRenderer2 = (UpsampleBlendRenderer*)m_upsampleBlendRenderer2->m_renderer;
		UpsampleBlendRenderer* lv_upsampleRenderer3 = (UpsampleBlendRenderer*)m_upsampleBlendRenderer3->m_renderer;


		lv_upsampleRenderer0->SetRadius(m_upsampleRadius);
		lv_upsampleRenderer1->SetRadius(m_upsampleRadius);
		lv_upsampleRenderer2->SetRadius(m_upsampleRadius);
		lv_upsampleRenderer3->SetRadius(m_upsampleRadius);

	}


	void IMGUIRenderer::DisableShadow()
	{
	

		m_omniDirectional0->m_enabled = false;
		m_omniDirectional1->m_enabled = false;
		m_omniDirectional2->m_enabled = false;
		m_omniDirectional3->m_enabled = false;
		m_omniDirectional4->m_enabled = false;
		m_omniDirectional5->m_enabled = false;


	}


	void IMGUIRenderer::SwitchToTiledDeferred()
	{

		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();
		PresentSwapchainRenderer* lv_fxxaaRenderer = (PresentSwapchainRenderer*)m_fxxaRenderer->m_renderer;
		TiledDeferredLightningRenderer* lv_tiledDeferred = (TiledDeferredLightningRenderer*)m_tiledDeferredLightningRenderer->m_renderer;

		if (m_switchToTiledDeferrred == true) {

			if (m_cachedSwitchToTiledDeferred == false) {

				m_tiledDeferredLightningRenderer->m_enabled = true;
				m_deferredLightningRenderer->m_enabled = false;
				m_cachedSwitchToTiledDeferred = true;
			}


				if (m_switchToDebugTiledDeferred == true) {


					if (m_cacheSwitchToDebugTiledDeferred == false) {
						lv_frameGraph.DisableNodesAfterGivenNodeHandleUntilLast2(m_tiledDeferredSortedHandle);
						lv_fxxaaRenderer->UpdateInputDescriptorImages(m_tiledDeferredOutput);
						lv_fxxaaRenderer->SetSwitchToDebugTiled(true);
						lv_tiledDeferred->SetSwitchToDebugTiled(true);
						m_cacheSwitchToDebugTiledDeferred = true;
					}
				}
				else {
					lv_frameGraph.EnableAllNodes();
					DisableShadow();
					m_tiledDeferredLightningRenderer->m_enabled = true;
					m_deferredLightningRenderer->m_enabled = false;
					lv_fxxaaRenderer->UpdateInputDescriptorImages(m_fxaaInput);
					lv_fxxaaRenderer->SetSwitchToDebugTiled(false);
					lv_tiledDeferred->SetSwitchToDebugTiled(false);
					m_cacheSwitchToDebugTiledDeferred = false;

				}


		}
		else {
			if (m_cachedSwitchToTiledDeferred == true) {
				lv_frameGraph.EnableAllNodes();
				DisableShadow();
				m_tiledDeferredLightningRenderer->m_enabled = false;
				m_cachedSwitchToTiledDeferred = false;
				m_cacheSwitchToDebugTiledDeferred = false;
				lv_fxxaaRenderer->UpdateInputDescriptorImages(m_fxaaInput);
				lv_fxxaaRenderer->SetSwitchToDebugTiled(false);
				lv_tiledDeferred->SetSwitchToDebugTiled(false);
			}
		}
	}

	void IMGUIRenderer::UpdateSSAOUniform()
	{
		SSAORenderer::UniformBufferMatrices lv_newUniform{};
		lv_newUniform.m_offsetBufferSize = m_offsetBufferSize;
		lv_newUniform.m_radius = m_radiusSSAO;

		SSAORenderer* lv_ssao = (SSAORenderer*)m_ssaoRenderer->m_renderer;

		lv_ssao->SetUniformBuffer(lv_newUniform);

	}

	void IMGUIRenderer::FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
		uint32_t l_currentSwapchainIndex)
	{

		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();


		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		static bool lv_showDemo = true;
		//static bool lv_showAnotherWindow = true;

		if (true == lv_showDemo) {
			ImGui::ShowDemoWindow(&lv_showDemo);
		}

		std::array<float, 4> lv_tempClearColor{ 0.45f, 0.55f, 0.60f, 1.00f };

		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Scene data");                          // Create a window called "Hello, world!" and append into it.

			//ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
			//ImGui::Checkbox("Demo Window", &lv_showDemo);      // Edit bools storing our window open/close state
			//ImGui::Checkbox("Another Window", &lv_showAnotherWindow);

			//ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			//ImGui::ColorEdit3("clear color", lv_tempClearColor.data()); // Edit 3 floats representing a color

			//if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			//	counter++;
			//ImGui::SameLine();
			//ImGui::Text("counter = %d", counter);


			ImGui::Text("Frustum culling\n");
			ImGui::Text("There are %u visible meshes", m_totalNumVisibleMeshes);


			ImGui::Text("\nTiled Deferred Lightning\n");
			ImGui::Checkbox("Switch to tiled deferred lightning", &m_switchToTiledDeferrred);
			ImGui::Checkbox("Switch to debug tiled deferred lightning", &m_switchToDebugTiledDeferred);

			if (false == m_switchToTiledDeferrred) {
				m_switchToDebugTiledDeferred = false;
			}

			ImGui::Text("\n");
			ImGui::Text("SSAO");
			ImGui::SliderFloat("Radius", &m_radiusSSAO, 0.1f, 15.0f);
			ImGui::SliderInt("OffsetBufferSize", &m_offsetBufferSize, 1.f, 64.0f);
			ImGui::Checkbox("Show oclusion factor image without blur", &m_showSSAOTextureOnly);

			if (true == m_switchToTiledDeferrred) {
				m_showSSAOTextureOnly = false;
			}

			ImGui::Text("\nBloom");
			ImGui::SliderFloat("RadiusUpsample", &m_upsampleRadius, 0.001f, 0.02f, "%.5f");


			ImGui::Text("\nPoint light cube");
			ImGui::SliderFloat("Light intensity", &m_lightIntensity, 1000.f , 25000.f);

			ImGui::Text("\nFPS");
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / m_io->Framerate, m_io->Framerate);
			ImGui::End();
		}

		//if(true == lv_showAnotherWindow)
		//{
		//	ImGui::Begin("Another Window", &lv_showAnotherWindow);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		//	ImGui::Text("Hello from another window!");
		//	if (ImGui::Button("Close Me"))
		//		lv_showAnotherWindow = false;
		//	ImGui::End();
		//}




		// Rendering
		ImGui::Render();
		ImDrawData* draw_data = ImGui::GetDrawData();
		const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
		
			
		auto lv_framebuffer = lv_vkResManager.RetrieveGpuFramebuffer(m_framebufferHandles[l_currentSwapchainIndex]);


		VkClearValue lv_clear{};
		lv_clear.color.float32[0] = lv_tempClearColor[0];
		lv_clear.color.float32[1] = lv_tempClearColor[1];
		lv_clear.color.float32[2] = lv_tempClearColor[2];
		lv_clear.color.float32[3] = lv_tempClearColor[3];

		VkRenderPassBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		info.renderPass = m_renderPass;
		info.framebuffer = lv_framebuffer;
		info.renderArea.extent.width = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferWidth;
		info.renderArea.extent.height = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferHeight;
		info.clearValueCount = 1;
		info.pClearValues = &lv_clear;
		vkCmdBeginRenderPass(l_cmdBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);

		ImGui_ImplVulkan_RenderDrawData(draw_data, l_cmdBuffer);
			
		vkCmdEndRenderPass(l_cmdBuffer);

		
		m_swapchains[l_currentSwapchainIndex]->Layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}


	void IMGUIRenderer::UpdateBuffers(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure)
	{
		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();
		PresentSwapchainRenderer* lv_fxxaaRenderer = (PresentSwapchainRenderer*)m_fxxaRenderer->m_renderer;
		TiledDeferredLightningRenderer* lv_tiledDeferred = (TiledDeferredLightningRenderer*)m_tiledDeferredLightningRenderer->m_renderer;
		SingleModelRenderer* lv_pointLightCube = (SingleModelRenderer*)m_pointLightCubeRenderer->m_renderer;
		DeferredLightningRenderer* lv_deferredLightning = (DeferredLightningRenderer*)m_deferredLightningRenderer->m_renderer;

		m_io->DisplaySize = ImVec2((float)m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferWidth, (float)m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferHeight);
		UpdateIncomingDataFromNodes();

		UpdateSSAOUniform();

		if (true == m_showSSAOTextureOnly && false == m_switchToTiledDeferrred) {
			if (false == m_cachedShowSSAOTextureOnly) {
				lv_frameGraph.DisableNodesAfterGivenNodeHandleUntilLast2(m_ssaoSortedHandle);
				lv_fxxaaRenderer->UpdateInputDescriptorImages(m_ssaoTextures);
				m_cachedShowSSAOTextureOnly = true;
			}
		}
		else if(true == m_cachedShowSSAOTextureOnly){
			lv_frameGraph.EnableAllNodes();
			m_tiledDeferredLightningRenderer->m_enabled = false;
			lv_fxxaaRenderer->UpdateDescriptorSets();
			m_cachedShowSSAOTextureOnly = false;
		}


		UpdateRadiusUpsamples();

		SwitchToTiledDeferred();

		lv_pointLightCube->SetLightIntensity(m_lightIntensity);
		lv_deferredLightning->SetPointLightIntensity(m_lightIntensity);
	}


	void IMGUIRenderer::UpdateDescriptorSets()
	{

	}



	IMGUIRenderer::~IMGUIRenderer()
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}


}