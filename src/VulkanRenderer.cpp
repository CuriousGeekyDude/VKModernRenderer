


#include "VulkanRenderer.hpp"
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "CameraStructure.hpp"


namespace VulkanEngine
{

	VulkanRenderer::VulkanRenderer(int l_width, int l_height, const std::string& l_frameGraphPath)
		:CameraApp(l_width, l_height, l_frameGraphPath),
		m_indirectGbuffer(ctx_,
			ctx_.GetContextCreator().m_vkDev.m_mainCommandBuffers2[0], "InitFiles/Binary Scene Files/SponzaMeshFileHeader"
			, "InitFiles/Binary Scene Files/SponzaBoundingBoxes", "InitFiles/Binary Scene Files/SponzaInstanceData",
			"InitFiles/Binary Scene Files/SponzaMaterialFile",
			"InitFiles/Binary Scene Files/SponzaSceneFile",
			"Shaders/Indirect.vert",
			"Shaders/Indirect.frag",
			"Shaders/Spirv/Indirect.spv"),
		m_boundingBoxWireframe(ctx_,
			"Shaders/BoundingBoxWireframe.vert", "Shaders/BoundingBoxWireframe.frag",
			"Shaders/Spirv/BoundingBox.spv")
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