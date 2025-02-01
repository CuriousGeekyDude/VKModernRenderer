



#include "VulkanContextCreator.hpp"
#include <glfw/glfw3.h>


namespace VulkanEngine
{
	VulkanContextCreator::VulkanContextCreator(void* window, int screenWidth, int screenHeight,
		const VulkanContextFeatures& ctxFeatures)
	{
		createInstance(&m_vulkanInstance.instance);

		if (!setupDebugCallbacks(m_vulkanInstance.instance, &m_vulkanInstance.messenger, &m_vulkanInstance.reportCallback))
			exit(0);

		if (glfwCreateWindowSurface(m_vulkanInstance.instance, (GLFWwindow*)window, nullptr, &m_vulkanInstance.surface))
			exit(0);

		if (!initVulkanRenderDevice3(m_vulkanInstance, m_vkDev, screenWidth, screenHeight, ctxFeatures))
			exit(0);
	}

	VulkanContextCreator::~VulkanContextCreator()
	{
		destroyVulkanRenderDevice(m_vkDev);
		destroyVulkanInstance(m_vulkanInstance);
	}
}