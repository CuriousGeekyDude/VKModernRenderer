#pragma once 



#include "VulkanEngineCore.hpp"
#include "IndirectRenderer.hpp"
//#include "SSAORenderer.hpp"
#include <optional>
#include "BoundingBoxWireframeRenderer.hpp"
#include "FrameGraph.hpp"
#include "DeferredLightningRenderer.hpp"
#include "SSAORenderer.hpp"
#include "BoxBlurRenderer.hpp"
#include "ClearSwapchainDepthRenderer.hpp"
#include "DepthMapLightRenderer.hpp"
#include "SingleModelRenderer.hpp"
#include "ExtractBrightnessRenderer.hpp"
#include "GaussianBlurRenderer.hpp"
#include "BloomBlendBlurAndSceneRenderer.hpp"
#include "WireframeDebugTiledDeferredRenderer.hpp"

namespace VulkanEngine
{
	class VulkanRenderer : public CameraApp
	{
	public:
		VulkanRenderer(int l_width, int l_height, const std::string& l_frameGraphPath);

	protected:
		virtual void update(float deltaSeconds) { CameraApp::update(deltaSeconds); }

		virtual void draw3D(uint32_t l_currentImageIndex) override;

	private:
		//RenderCore::PresentToColorAttachRenderer m_presentToColor;
		RenderCore::ClearSwapchainDepthRenderer m_clearSwapchainDepth;
		RenderCore::IndirectRenderer m_indirectGbuffer;
		//RenderCore::BoundingBoxWireframeRenderer m_boundingBoxWireframe;
		RenderCore::SSAORenderer m_ssao;
		RenderCore::BoxBlurRenderer m_boxBlur;
		RenderCore::DepthMapLightRenderer m_depthMapLightPlusX;
		RenderCore::DepthMapLightRenderer m_depthMapLightMinusX;
		RenderCore::DepthMapLightRenderer m_depthMapLightPlusY;
		RenderCore::DepthMapLightRenderer m_depthMapLightMinusY;
		RenderCore::DepthMapLightRenderer m_depthMapLightPlusZ;
		RenderCore::DepthMapLightRenderer m_depthMapLightMinusZ;
		RenderCore::DeferredLightningRenderer m_deferredLightning;
		RenderCore::SingleModelRenderer m_pointLightCube;
		RenderCore::ExtractBrightnessRenderer m_extractBrightnessBloom;

		RenderCore::GaussianBlurRenderer m_blurHor0;
		RenderCore::GaussianBlurRenderer m_blurHor1;
		RenderCore::GaussianBlurRenderer m_blurHor2;
		RenderCore::GaussianBlurRenderer m_blurHor3;
		RenderCore::GaussianBlurRenderer m_blurHor4;


		RenderCore::GaussianBlurRenderer m_blurVert0;
		RenderCore::GaussianBlurRenderer m_blurVert1;
		RenderCore::GaussianBlurRenderer m_blurVert2;
		RenderCore::GaussianBlurRenderer m_blurVert3;
		RenderCore::GaussianBlurRenderer m_blurVert4;

		RenderCore::BloomBlendBlurAndSceneRenderer m_bloomBlend;

		RenderCore::WireframeDebugTiledDeferredRenderer m_debugTiled;

	};
}