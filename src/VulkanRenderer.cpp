


#include "VulkanRenderer.hpp"
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "CameraStructure.hpp"


namespace VulkanEngine
{

	VulkanRenderer::VulkanRenderer(int l_width, int l_height, const std::string& l_frameGraphPath)
		:CameraApp(l_width, l_height, l_frameGraphPath),
		m_clearSwapchainDepth(ctx_),
		m_depthMapLightPlusX(ctx_, "Shaders/DepthMapLight.vert", "Shaders/DepthMapLight.frag", "Shaders/Spirv/DepthMapLight.spv", "DepthMapOmnidirectionalPointLight0", glm::vec3{ -6.f, 13.f, -2.f }, glm::vec3{ -6.f, 13.f, -2.f } + glm::vec3{ 1.f, 0.f, 0.f }, glm::vec3{0.f, -1.f, 0.f}, 0),
		m_depthMapLightMinusX(ctx_, "Shaders/DepthMapLight.vert", "Shaders/DepthMapLight.frag", "Shaders/Spirv/DepthMapLight.spv", "DepthMapOmnidirectionalPointLight1", glm::vec3{ -6.f, 13.f, -2.f }, glm::vec3{ -6.f, 13.f, -2.f } + glm::vec3{ -1.f, 0.f, 0.f }, glm::vec3{ 0.f, -1.f, 0.f }, 1),
		m_depthMapLightPlusY(ctx_, "Shaders/DepthMapLight.vert", "Shaders/DepthMapLight.frag", "Shaders/Spirv/DepthMapLight.spv", "DepthMapOmnidirectionalPointLight2", glm::vec3{ -6.f, 13.f, -2.f }, glm::vec3{ -6.f, 13.f, -2.f } + glm::vec3{ 0.f, 1.f, 0.f }, glm::vec3{ 0.f, 0.f, 1.f }, 2),
		m_depthMapLightMinusY(ctx_, "Shaders/DepthMapLight.vert", "Shaders/DepthMapLight.frag", "Shaders/Spirv/DepthMapLight.spv", "DepthMapOmnidirectionalPointLight3", glm::vec3{ -6.f, 13.f, -2.f }, glm::vec3{ -6.f, 13.f, -2.f } + glm::vec3{ 0.f, -1.f, 0.f }, glm::vec3{ 0.f, 0.f, -1.f }, 3),
		m_depthMapLightPlusZ(ctx_, "Shaders/DepthMapLight.vert", "Shaders/DepthMapLight.frag", "Shaders/Spirv/DepthMapLight.spv", "DepthMapOmnidirectionalPointLight4", glm::vec3{ -6.f, 13.f, -2.f }, glm::vec3{ -6.f, 13.f, -2.f } + glm::vec3{ 0.f, 0.f, 1.f }, glm::vec3{ 0.f, -1.f, 0.f }, 4),
		m_depthMapLightMinusZ(ctx_, "Shaders/DepthMapLight.vert", "Shaders/DepthMapLight.frag", "Shaders/Spirv/DepthMapLight.spv", "DepthMapOmnidirectionalPointLight5", glm::vec3{ -6.f, 13.f, -2.f }, glm::vec3{ -6.f, 13.f, -2.f } + glm::vec3{ 0.f, 0.f, -1.f }, glm::vec3{ 0.f, -1.f, 0.f }, 5),
		m_indirectGbuffer(ctx_,
			ctx_.GetContextCreator().m_vkDev.m_mainCommandBuffers2[0], "InitFiles/Binary Scene Files/SponzaMeshFileHeader"
			, "InitFiles/Binary Scene Files/SponzaBoundingBoxes", "InitFiles/Binary Scene Files/SponzaInstanceData",
			"InitFiles/Binary Scene Files/SponzaMaterialFile",
			"InitFiles/Binary Scene Files/SponzaSceneFile",
			"Shaders/Indirect.vert",
			"Shaders/Indirect.frag",
			"Shaders/Spirv/Indirect.spv")
		/*m_boundingBoxWireframe(ctx_,
			"Shaders/BoundingBoxWireframe.vert", "Shaders/BoundingBoxWireframe.frag",
			"Shaders/Spirv/BoundingBox.spv")*/
		, m_ssao(ctx_, "Shaders/FullScreenQuad.vert", "Shaders/SSAO.frag"
			, "Shaders/Spirv/SSAO.spv")
		, m_boxBlur(ctx_, "Shaders/FullScreenQuad.vert", "Shaders/BoxBlur.frag"
					, "Shaders/Spirv/BoxBlur.spv")
		,m_deferredLightning(ctx_
		,"Shaders/FullScreenQuad.vert"
		,"Shaders/DeferredLightning.frag"
		,"Shaders/Spirv/DeferredLightning.spv")
		/*m_interior(ctx_, ctx_.GetOffscreenRenderPassDepth(),
			ctx_.GetContextCreator().m_vkDev.m_graphicsCommandBuffers[0], "build/Chapter3/VK02_DemoApp/CustomSceneStructures/BristoInteriorMeshFileHeader"
		,"build/Chapter3/VK02_DemoApp/CustomSceneStructures/BristoInteriorBoundingBoxes", "build/Chapter3/VK02_DemoApp/CustomSceneStructures/BristoInteriorInstanceData",
		 "build/Chapter3/VK02_DemoApp/CustomSceneStructures/BristoInteriorMaterialFile", "build/Chapter3/VK02_DemoApp/CustomSceneStructures/BristoInteriorSceneFile", "C:/Users/farhan/source/repos/3D-Graphics-Rendering-Cookbook-master/data/shaders/chapter05/VK01.vert", "C:/Users/farhan/source/repos/3D-Graphics-Rendering-Cookbook-master/data/shaders/chapter05/VK01.frag"),*/
		/*m_exterior(ctx_,
			ctx_.GetContextCreator().m_vkDev.m_mainCommandBuffers2[0], "build/Chapter3/VK02_DemoApp/CustomSceneStructures/SponzaMeshFileHeader"
			, "build/Chapter3/VK02_DemoApp/CustomSceneStructures/SponzaBoundingBoxes", "build/Chapter3/VK02_DemoApp/CustomSceneStructures/SponzaInstanceData",
			"build/Chapter3/VK02_DemoApp/CustomSceneStructures/SponzaMaterialFile", 
			"build/Chapter3/VK02_DemoApp/CustomSceneStructures/SponzaSceneFile" ,
			"C:/Users/farhan/source/repos/3D-Graphics-Rendering-Cookbook-master/build/Chapter3/VK02_DemoApp/Shaders/VK01.vert",
			"C:/Users/farhan/source/repos/3D-Graphics-Rendering-Cookbook-master/build/Chapter3/VK02_DemoApp/Shaders/VK01.frag",
			"build/Chapter3/VK02_DemoApp/Shaders/Indirect.spv"),
		m_deferred(ctx_, 
			"C:/Users/farhan/source/repos/3D-Graphics-Rendering-Cookbook-master/build/Chapter3/VK02_DemoApp/Shaders/VK02_Quad.vert", 
			"C:/Users/farhan/source/repos/3D-Graphics-Rendering-Cookbook-master/build/Chapter3/VK02_DemoApp/Shaders/Deferred.frag",
			"build/Chapter3/VK02_DemoApp/Shaders/Deferred.spv")*/
	{


		
		////ctx_.m_offScreenRenderers.emplace_back(m_interior, true, true);
		//ctx_.m_offScreenRenderers.emplace_back(m_exterior, true, true);
		//ctx_.m_offScreenRenderers.emplace_back(m_deferred, true, false);
	}

	void VulkanRenderer::draw3D(uint32_t l_currentImageIndex)
	{
		const float lv_ratio = (float)ctx_.GetContextCreator().m_vkDev.m_framebufferWidth / (float)ctx_.GetContextCreator().m_vkDev.m_framebufferHeight;
		auto proj = glm::perspective((float)glm::radians(60.f), lv_ratio, 0.01f, 1000.f);


		const CameraStructure lv_cameraStructure{
			.m_cameraPos = camera.getPosition(),
			.m_viewMatrix = camera.getViewMatrix(),
			.m_projectionMatrix = proj
		};

		ctx_.UpdateRenderers(l_currentImageIndex, lv_cameraStructure);
	}

}