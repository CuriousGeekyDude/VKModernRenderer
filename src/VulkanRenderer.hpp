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


//Bloom effect sub-effects
#include "DownsampleToMipmapsRenderer.hpp"
#include "UpsampleBlendRenderer.hpp"
#include "LinearlyInterpBlurAndSceneRenderer.hpp"


#include "FindingMaxMinDepthOfEachTile.hpp"

#include "PresentSwapchainRenderer.hpp"

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

		RenderCore::DownsampleToMipmapsRenderer m_downsampleToMipmaps0;
		RenderCore::DownsampleToMipmapsRenderer m_downsampleToMipmaps1;
		RenderCore::DownsampleToMipmapsRenderer m_downsampleToMipmaps2;
		RenderCore::DownsampleToMipmapsRenderer m_downsampleToMipmaps3;
		RenderCore::DownsampleToMipmapsRenderer m_downsampleToMipmaps4;

		RenderCore::UpsampleBlendRenderer m_upsampleBlend4;
		RenderCore::UpsampleBlendRenderer m_upsampleBlend3;
		RenderCore::UpsampleBlendRenderer m_upsampleBlend2;
		RenderCore::UpsampleBlendRenderer m_upsampleBlend1;
		RenderCore::LinearlyInterpBlurAndSceneRenderer m_linearlyInterpBlurScene;


		RenderCore::PresentSwapchainRenderer m_presentSwapchain;


		RenderCore::FindingMaxMinDepthOfEachTile m_minMaxDepthTiles;

	};
}