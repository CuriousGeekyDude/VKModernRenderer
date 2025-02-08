#pragma once


#include "glm/glm.hpp"


namespace VulkanEngine
{
	struct CameraStructure
	{
		glm::vec3 m_cameraPos;
		glm::mat4 m_viewMatrix;
		glm::mat4 m_projectionMatrix;
	};
}