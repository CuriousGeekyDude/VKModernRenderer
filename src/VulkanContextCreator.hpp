#pragma once


#include "UtilsVulkan.h"


namespace VulkanEngine
{
	struct VulkanContextCreator final
	{
		VulkanContextCreator() = default;

		VulkanContextCreator(void* window, int screenWidth, int screenHeight, 
			const VulkanContextFeatures& ctxFeatures = VulkanContextFeatures());
		~VulkanContextCreator();

		VulkanInstance m_vulkanInstance{};
		VulkanRenderDevice m_vkDev{};

		static const uint32_t m_monitorMaxWidth{1366};
		static const uint32_t m_monitorMaxHeight{768};
	};
}