#pragma once 



#include "VulkanEngineCore.hpp"
#include "IndirectRenderer.hpp"
//#include "SSAORenderer.hpp"
#include <optional>
#include "DeferredComputeRenderer.hpp"
#include "DeferredForwardRenderer.hpp"
#include "FrameGraph.hpp"

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
		RenderCore::IndirectRenderer m_indirectGbuffer;
		RenderCore::DeferredComputeRenderer m_deferredCompute;
		RenderCore::DeferredForwardRenderer m_deferredForward;


	};
}