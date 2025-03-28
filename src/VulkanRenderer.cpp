


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
		m_depthMapLightPlusX(ctx_, "Shaders/DepthMapLight.vert", "Shaders/DepthMapLight.frag", "Shaders/Spirv/DepthMapLight.spv", "DepthMapOmnidirectionalPointLight0", glm::vec3{ -13.f, 18.f, -2.f }, glm::vec3{ -13.f, 18.f, -2.f } + glm::vec3{ 1.f, 0.f, 0.f }, glm::vec3{0.f, -1.f, 0.f}, 0),
		m_depthMapLightMinusX(ctx_, "Shaders/DepthMapLight.vert", "Shaders/DepthMapLight.frag", "Shaders/Spirv/DepthMapLight.spv", "DepthMapOmnidirectionalPointLight1", glm::vec3{ -13.f, 18.f, -2.f }, glm::vec3{ -13.f, 18.f, -2.f } + glm::vec3{ -1.f, 0.f, 0.f }, glm::vec3{ 0.f, -1.f, 0.f }, 1),
		m_depthMapLightPlusY(ctx_, "Shaders/DepthMapLight.vert", "Shaders/DepthMapLight.frag", "Shaders/Spirv/DepthMapLight.spv", "DepthMapOmnidirectionalPointLight2", glm::vec3{ -13.f, 18.f, -2.f }, glm::vec3{ -13.f, 18.f, -2.f } + glm::vec3{ 0.f, 1.f, 0.f }, glm::vec3{ 0.f, 0.f, 1.f }, 2),
		m_depthMapLightMinusY(ctx_, "Shaders/DepthMapLight.vert", "Shaders/DepthMapLight.frag", "Shaders/Spirv/DepthMapLight.spv", "DepthMapOmnidirectionalPointLight3", glm::vec3{ -13.f, 18.f, -2.f }, glm::vec3{ -13.f, 18.f, -2.f } + glm::vec3{ 0.f, -1.f, 0.f }, glm::vec3{ 0.f, 0.f, -1.f }, 3),
		m_depthMapLightPlusZ(ctx_, "Shaders/DepthMapLight.vert", "Shaders/DepthMapLight.frag", "Shaders/Spirv/DepthMapLight.spv", "DepthMapOmnidirectionalPointLight4", glm::vec3{ -13.f, 18.f, -2.f }, glm::vec3{ -13.f, 18.f, -2.f } + glm::vec3{ 0.f, 0.f, 1.f }, glm::vec3{ 0.f, -1.f, 0.f }, 4),
		m_depthMapLightMinusZ(ctx_, "Shaders/DepthMapLight.vert", "Shaders/DepthMapLight.frag", "Shaders/Spirv/DepthMapLight.spv", "DepthMapOmnidirectionalPointLight5", glm::vec3{ -13.f, 18.f, -2.f }, glm::vec3{ -13.f, 18.f, -2.f } + glm::vec3{ 0.f, 0.f, -1.f }, glm::vec3{ 0.f, -1.f, 0.f }, 5),
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
		
		,m_pointLightCube(ctx_)
		,m_extractBrightnessBloom(ctx_
		 ,"Shaders/FullScreenQuad.vert"
		, "Shaders/ExtractBrightness.frag"
		,"Shaders/Spirv/ExtractBrightness.spv")

		,m_blurHor0(ctx_, "Shaders/FullScreenQuad.vert", "Shaders/GaussianBlurHorizontal.frag", "Shaders/Spirv/GaussianBlur.spv", 0, true, "GaussianBlurHorizontal0")
		,m_blurHor1(ctx_, "Shaders/FullScreenQuad.vert", "Shaders/GaussianBlurHorizontal.frag", "Shaders/Spirv/GaussianBlur.spv", 1, false, "GaussianBlurHorizontal1")
		,m_blurHor2(ctx_, "Shaders/FullScreenQuad.vert", "Shaders/GaussianBlurHorizontal.frag", "Shaders/Spirv/GaussianBlur.spv", 0, false, "GaussianBlurHorizontal2")
		,m_blurHor3(ctx_, "Shaders/FullScreenQuad.vert", "Shaders/GaussianBlurHorizontal.frag", "Shaders/Spirv/GaussianBlur.spv", 1, false, "GaussianBlurHorizontal3")
		,m_blurHor4(ctx_, "Shaders/FullScreenQuad.vert", "Shaders/GaussianBlurHorizontal.frag", "Shaders/Spirv/GaussianBlur.spv", 0, false, "GaussianBlurHorizontal4")

		,m_blurVert0(ctx_, "Shaders/FullScreenQuad.vert", "Shaders/GaussianBlurVertical.frag", "Shaders/Spirv/GaussianBlur.spv", 1, false, "GaussianBlurVertical0")
		,m_blurVert1(ctx_, "Shaders/FullScreenQuad.vert", "Shaders/GaussianBlurVertical.frag", "Shaders/Spirv/GaussianBlur.spv", 0, false, "GaussianBlurVertical1")
		,m_blurVert2(ctx_, "Shaders/FullScreenQuad.vert", "Shaders/GaussianBlurVertical.frag", "Shaders/Spirv/GaussianBlur.spv", 1, false, "GaussianBlurVertical2")
		,m_blurVert3(ctx_, "Shaders/FullScreenQuad.vert", "Shaders/GaussianBlurVertical.frag", "Shaders/Spirv/GaussianBlur.spv", 0, false, "GaussianBlurVertical3")
		,m_blurVert4(ctx_, "Shaders/FullScreenQuad.vert", "Shaders/GaussianBlurVertical.frag", "Shaders/Spirv/GaussianBlur.spv", 1, false, "GaussianBlurVertical4")
		
		,m_bloomBlend(ctx_, "Shaders/FullScreenQuad.vert", "Shaders/BloomBlend.frag", "Shaders/Spirv/BloomBlend.spv", 1)
		//,m_debugTiled(ctx_, "Shaders/WireframeDebugTiledDeferred.vert", "Shaders/WireframeDebugTiledDeferred.frag", nullptr)

	{


		std::vector<glm::vec3> vertices{
			// Front face
			{ -0.5f, -0.5f,  0.5f }, // 0
			{  0.5f, -0.5f,  0.5f }, // 1
			{  0.5f,  0.5f,  0.5f }, // 2
			{ -0.5f,  0.5f,  0.5f }, // 3

			// Back face
			{ -0.5f, -0.5f, -0.5f }, // 4
			{  0.5f, -0.5f, -0.5f }, // 5
			{  0.5f,  0.5f, -0.5f }, // 6
			{ -0.5f,  0.5f, -0.5f }  // 7
		};

		glm::vec3 lv_lightPos{ -13.f, 18.f, -2.f };

		for (auto& l_pos : vertices) {
			l_pos *= 5.090127f;
			l_pos += lv_lightPos;
		}

		const std::vector<uint32_t> indices{
			// Front face
			0, 1, 2,  2, 3, 0,

			// Back face
			4, 5, 6,  6, 7, 4,

			// Left face
			4, 0, 3,  3, 7, 4,

			// Right face
			1, 5, 6,  6, 2, 1,

			// Top face
			3, 2, 6,  6, 7, 3,

			// Bottom face
			4, 5, 1,  1, 0, 4
		};


		m_pointLightCube.Init(vertices, indices, "Shaders/PointLightCube.vert", "Shaders/PointLightCube.frag", "Shaders/Spirv/PointLightCube.spv");
		
		////ctx_.m_offScreenRenderers.emplace_back(m_interior, true, true);
		//ctx_.m_offScreenRenderers.emplace_back(m_exterior, true, true);
		//ctx_.m_offScreenRenderers.emplace_back(m_deferred, true, false);
	}

	void VulkanRenderer::draw3D(uint32_t l_currentImageIndex)
	{
		const float lv_ratio = (float)ctx_.GetContextCreator().m_vkDev.m_framebufferWidth / (float)ctx_.GetContextCreator().m_vkDev.m_framebufferHeight;
		auto proj = glm::perspective((float)glm::radians(60.f), lv_ratio, 0.1f, 145.f);


		const CameraStructure lv_cameraStructure{
			.m_cameraPos = camera.getPosition(),
			.m_viewMatrix = camera.getViewMatrix(),
			.m_projectionMatrix = proj
		};

		ctx_.UpdateRenderers(l_currentImageIndex, lv_cameraStructure);
	}

}