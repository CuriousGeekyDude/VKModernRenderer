#pragma once


#include "VulkanRenderContext.hpp"
#include "Camera.h"
#include <glm/glm.hpp>
#include <glfw/glfw3.h>


namespace VulkanEngine
{

	class FramesPerSecondCounter
	{
	public:
		explicit FramesPerSecondCounter(float avgInterval = 0.5f)
			:avgInterval_(avgInterval)
		{
			assert(avgInterval > 0.0f);
		}

		bool tick(float deltaSeconds, bool frameRendered = true)
		{
			if (frameRendered)
				numFrames_++;

			accumulatedTime_ += deltaSeconds;

			if (accumulatedTime_ > avgInterval_)
			{
				currentFPS_ = static_cast<float>(numFrames_ / accumulatedTime_);
				if (printFPS_)
					printf("FPS: %.1f\n", currentFPS_);
				numFrames_ = 0;
				accumulatedTime_ = 0;
				return true;
			}

			return false;
		}

		inline float getFPS() const { return currentFPS_; }

		bool printFPS_ = true;

	private:
		const float avgInterval_ = 0.5f;
		unsigned int numFrames_ = 0;
		double accumulatedTime_ = 0;
		float currentFPS_ = 0.0f;
	};

	struct Resolution
	{
		uint32_t width = 0;
		uint32_t height = 0;
	};

	struct VulkanApp
	{
		

		VulkanApp(int screenWidth, int screenHeight, const std::string& l_frameGraph);

		~VulkanApp();

		virtual void drawUI() {}
		virtual void draw3D(uint32_t l_currentImageIndex) = 0;

		void mainLoop();

		// Check if none of the ImGui widgets were touched so our app can process mouse events
		bool shouldHandleMouse() const;

		virtual void handleKey(int key, bool pressed) = 0;
		virtual void handleMouseClick(int button, bool pressed);

		virtual void handleMouseMove(float mx, float my);

		virtual void update(float deltaSeconds) = 0;

		float getFPS() const;

		bool drawFrame(const std::function<void(uint32_t)>& updateBuffersFunc, 
			const std::function<void(VkCommandBuffer, uint32_t)>& composeFrameFunc);

	protected:
		struct MouseState
		{
			glm::vec2 pos = glm::vec2(0.0f);
			bool pressedLeft = false;
		} mouseState_;

		Resolution resolution_;
		GLFWwindow* window_ = nullptr;
		VulkanRenderContext ctx_;
		FramesPerSecondCounter fpsCounter_;

	private:
		void assignCallbacks();

		void updateBuffers(uint32_t imageIndex);

		Resolution detectResolution(int width, int height);

		GLFWwindow* initVulkanApp(int width, int height);
	};

	struct CameraApp : public VulkanApp
	{
		
		CameraApp(int screenWidth, int screenHeight, const std::string& l_frameGraphPath);

		virtual void update(float deltaSeconds) override;

		glm::mat4 getDefaultProjection();

		virtual void handleKey(int key, bool pressed) override;
		virtual void draw3D(uint32_t l_currentImageIndex) override {}

	protected:
		CameraPositioner_FirstPerson positioner;
		Camera camera;
	};
}