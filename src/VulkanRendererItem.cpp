



#include "VulkanRendererItem.hpp"
#include "Renderbase.hpp"


namespace RenderCore
{
	VulkanRendererItem::VulkanRendererItem(Renderbase& l_renderBase, bool l_enabled, bool l_useDepth)
		:m_rendererBase(l_renderBase), m_enabled(l_enabled), m_useDepth(l_useDepth) {}
}