#pragma once





namespace RenderCore
{

	class Renderbase;

	struct VulkanRendererItem
	{
		VulkanRendererItem(Renderbase& l_renderBase, bool l_enabled, bool l_useDepth);


		Renderbase& m_rendererBase;
		bool m_enabled{ false };
		bool m_useDepth{ false };
	};
}