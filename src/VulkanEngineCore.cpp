


#include "VulkanEngineCore.hpp"



namespace VulkanEngine
{


	VulkanApp::VulkanApp(int screenWidth, int screenHeight, const std::string& l_frameGraphPath)
		: window_(initVulkanApp(screenWidth, screenHeight)),
		ctx_(window_, resolution_.width, resolution_.height, l_frameGraphPath)
	{
		glfwSetWindowUserPointer(window_, this);
		assignCallbacks();
	}

	VulkanApp::~VulkanApp()
	{
		glslang_finalize_process();
		glfwTerminate();
	}



	void VulkanApp::handleMouseClick(int button, bool pressed)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT)
			mouseState_.pressedLeft = pressed;
	}


	void VulkanApp::handleMouseMove(float mx, float my)
	{
		mouseState_.pos.x = mx;
		mouseState_.pos.y = my;
	}


	Resolution VulkanApp::detectResolution(int width, int height)
	{
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const int code = glfwGetError(nullptr);

		if (code != 0)
		{
			printf("Monitor: %p; error = %x / %d\n", monitor, code, code);
			exit(255);
		}

		const GLFWvidmode* info = glfwGetVideoMode(monitor);

		const uint32_t windowW = width > 0 ? width : (uint32_t)(info->width * width / -100);
		const uint32_t windowH = height > 0 ? height : (uint32_t)(info->height * height / -100);

		return Resolution{ .width = windowW, .height = windowH };
	}


	bool VulkanApp::shouldHandleMouse() const { return false; /*return !ImGui::GetIO().WantCaptureMouse;*/ }
	float VulkanApp::getFPS() const { return fpsCounter_.getFPS(); }

	GLFWwindow* VulkanApp::initVulkanApp(int width, int height)
	{
		glslang_initialize_process();

		volkInitialize();

		if (!glfwInit())
			exit(EXIT_FAILURE);

		if (!glfwVulkanSupported())
			exit(EXIT_FAILURE);

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	
		resolution_ = detectResolution(width, height);
		width = resolution_.width;
		height = resolution_.height;
		

		GLFWwindow* result = glfwCreateWindow(width, height, "VulkanEngine", nullptr, nullptr);
		if (!result)
		{
			glfwTerminate();
			exit(EXIT_FAILURE);
		}

		return result;
	}

	bool VulkanApp::drawFrame(const std::function<void(uint32_t)>& updateBuffersFunc, const std::function<void(VkCommandBuffer, uint32_t)>& composeFrameFunc)
	{
		uint32_t imageIndex = 0;
		VkResult result = vkAcquireNextImageKHR(ctx_.GetContextCreator().m_vkDev.m_device, ctx_.GetContextCreator().m_vkDev.m_swapchain, 0, ctx_.GetContextCreator().m_vkDev.m_binarySemaphore, VK_NULL_HANDLE, &imageIndex);
		VK_CHECK(vkResetCommandPool(ctx_.GetContextCreator().m_vkDev.m_device, ctx_.GetContextCreator().m_vkDev.m_mainCommandPool2[0], 0));

		if (result != VK_SUCCESS) return false;

		updateBuffersFunc(imageIndex);


		//commandBuffer is not used. Will have to change signature of composeFrameFun()
		VkCommandBuffer commandBuffer{};


		composeFrameFunc(commandBuffer, imageIndex);


		//const VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; // or even VERTEX_SHADER_STAGE

		//const VkSubmitInfo si =
		//{
		//	.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		//	.pNext = nullptr,
		//	.waitSemaphoreCount = 1,
		//	.pWaitSemaphores = &ctx_.GetContextCreator().m_vkDev.m_binarySemaphore,
		//	.pWaitDstStageMask = waitStages,
		//	.commandBufferCount = 1,
		//	.pCommandBuffers = &ctx_.GetContextCreator().m_vkDev.m_mainCommandBuffers2[imageIndex],
		//	.signalSemaphoreCount = 1,
		//	.pSignalSemaphores = &ctx_.GetContextCreator().m_vkDev.m_binarySemaphore
		//};

		//VK_CHECK(vkQueueSubmit(ctx_.GetContextCreator().m_vkDev.m_mainQueue1, 1, &si, nullptr));

		//const VkPresentInfoKHR pi =
		//{
		//	.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		//	.pNext = nullptr,
		//	.waitSemaphoreCount = 1,
		//	.pWaitSemaphores = &ctx_.GetContextCreator().m_vkDev.m_binarySemaphore,
		//	.swapchainCount = 1,
		//	.pSwapchains = &ctx_.GetContextCreator().m_vkDev.m_swapchain,
		//	.pImageIndices = &imageIndex
		//};

		//VK_CHECK(vkQueuePresentKHR(ctx_.GetContextCreator().m_vkDev.m_mainQueue1, &pi));
		//VK_CHECK(vkDeviceWaitIdle(ctx_.GetContextCreator().m_vkDev.m_device));

		return true;
	}


	void VulkanApp::assignCallbacks()
	{
		glfwSetCursorPosCallback(
			window_,
			[](GLFWwindow* window, double x, double y)
			{
				//ImGui::GetIO().MousePos = ImVec2((float)x, (float)y);
				int width, height;
				glfwGetFramebufferSize(window, &width, &height);

				void* ptr = glfwGetWindowUserPointer(window);
				const float mx = static_cast<float>(x / width);
				const float my = static_cast<float>(y / height);
				reinterpret_cast<VulkanApp*>(ptr)->handleMouseMove(mx, my);
			}
		);

		glfwSetMouseButtonCallback(
			window_,
			[](GLFWwindow* window, int button, int action, int mods)
			{
				/*auto& io = ImGui::GetIO();
				const int idx = button == GLFW_MOUSE_BUTTON_LEFT ? 0 : button == GLFW_MOUSE_BUTTON_RIGHT ? 2 : 1;
				io.MouseDown[idx] = action == GLFW_PRESS;*/

				void* ptr = glfwGetWindowUserPointer(window);
				reinterpret_cast<VulkanApp*>(ptr)->handleMouseClick(button, action == GLFW_PRESS);
			}
		);

		glfwSetKeyCallback(
			window_,
			[](GLFWwindow* window, int key, int scancode, int action, int mods)
			{
				const bool pressed = action != GLFW_RELEASE;
				if (key == GLFW_KEY_ESCAPE && pressed)
					glfwSetWindowShouldClose(window, GLFW_TRUE);

				void* ptr = glfwGetWindowUserPointer(window);
				reinterpret_cast<VulkanApp*>(ptr)->handleKey(key, pressed);
			}
		);
	}

	void VulkanApp::updateBuffers(uint32_t imageIndex)
	{
		/*ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)ctx_.GetContextCreator().m_vkDev.m_framebufferWidth, (float)ctx_.GetContextCreator().m_vkDev.m_framebufferHeight);
		ImGui::NewFrame();

		drawUI();

		ImGui::Render();*/

		draw3D(imageIndex);

	}

	void VulkanApp::mainLoop()
	{
		double timeStamp = glfwGetTime();
		float deltaSeconds = 0.0f;

		do
		{
			update(deltaSeconds);

			const double newTimeStamp = glfwGetTime();
			deltaSeconds = static_cast<float>(newTimeStamp - timeStamp);
			timeStamp = newTimeStamp;

			fpsCounter_.tick(deltaSeconds);

			bool frameRendered = drawFrame(
				[this](uint32_t img) { this->updateBuffers(img); },
				[this](auto cmd, auto img) { ctx_.CreateFrame(cmd, img); }
			);

			fpsCounter_.tick(deltaSeconds, frameRendered);

			glfwPollEvents();

		} while (!glfwWindowShouldClose(window_));
	}











	CameraApp::CameraApp(int screenWidth, int screenHeight, const std::string& l_frameGraphPath) :
		VulkanApp(screenWidth, screenHeight, l_frameGraphPath),

		positioner(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
		camera(positioner)
	{}


	void CameraApp::update(float deltaSeconds)
	{
		positioner.update(deltaSeconds, mouseState_.pos, mouseState_.pressedLeft );
	}

	glm::mat4 CameraApp::getDefaultProjection() {
		const float ratio = ctx_.GetContextCreator().m_vkDev.m_framebufferWidth / (float)ctx_.GetContextCreator().m_vkDev.m_framebufferHeight;
		return glm::perspective(45.0f, ratio, 0.1f, 1000.0f);
	}


	void CameraApp::handleKey(int key, bool pressed)
	{
		if (key == GLFW_KEY_W)
			positioner.movement_.forward_ = pressed;
		if (key == GLFW_KEY_S)
			positioner.movement_.backward_ = pressed;
		if (key == GLFW_KEY_A)
			positioner.movement_.left_ = pressed;
		if (key == GLFW_KEY_D)
			positioner.movement_.right_ = pressed;
		if (key == GLFW_KEY_C)
			positioner.movement_.up_ = pressed;
		if (key == GLFW_KEY_E)
			positioner.movement_.down_ = pressed;
	}
}