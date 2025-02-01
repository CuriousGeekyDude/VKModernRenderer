


#include "VulkanCubeRenderer.hpp"
#include "ErrorCheck.hpp"

namespace RenderCore
{
	VulkanCubeRenderer::VulkanCubeRenderer(VulkanRenderDevice& l_renderDevice, VulkanImage l_depth,
		const char* l_textureFile) : Renderbase(l_renderDevice, l_depth)
	{
		createCubeTextureImage(l_renderDevice, l_textureFile, m_cubeTexture.image, m_cubeTexture.imageMemory);

		createImageView(l_renderDevice.m_device, m_cubeTexture.image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 
			&m_cubeTexture.imageView, VK_IMAGE_VIEW_TYPE_CUBE, 6);
		createTextureSampler(l_renderDevice.m_device, &m_cubeTextureSampler);
	

		if (!createColorAndDepthRenderPass(l_renderDevice, true, &m_renderPass, RenderPassCreateInfo()) ||
			!CreateUniformBuffers(l_renderDevice, sizeof(glm::mat4)) ||
			!createColorAndDepthFramebuffers(l_renderDevice, m_renderPass, 
				l_depth.imageView, m_frameBuffers) ||
			!createDescriptorPool(l_renderDevice, 1, 0, 1, &m_descriptorPool) ||
			!CreateDescriptorSet(l_renderDevice) ||
			!createPipelineLayout(l_renderDevice.m_device, m_descriptorSetLayout, &m_pipelineLayout) ||
			!createGraphicsPipeline(l_renderDevice, m_renderPass, m_pipelineLayout,
				{ "data/shaders/chapter04/VKCube.vert", "data/shaders/chapter04/VKCube.frag" }, &m_graphicsPipeline))
		{
			printf("CubeRenderer: failed to create pipeline\n");
			exit(EXIT_FAILURE);
		}
	}



	bool VulkanCubeRenderer::CreateDescriptorSet(VulkanRenderDevice& l_renderDevice)
	{
		using namespace ErrorCheck;

		const std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
		descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
		descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		const VkDescriptorSetLayoutCreateInfo layoutInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.bindingCount = static_cast<uint32_t>(bindings.size()),
			.pBindings = bindings.data()
		};

		VULKAN_CHECK(vkCreateDescriptorSetLayout(l_renderDevice.m_device, &layoutInfo,
			nullptr, &m_descriptorSetLayout));

		std::vector<VkDescriptorSetLayout> layouts(l_renderDevice.m_swapchainImages.size(), 
			m_descriptorSetLayout);

		const VkDescriptorSetAllocateInfo allocInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = nullptr,
			.descriptorPool = m_descriptorPool,
			.descriptorSetCount = static_cast<uint32_t>(l_renderDevice.m_swapchainImages.size()),
			.pSetLayouts = layouts.data()
		};

		m_descriptorSet.resize(l_renderDevice.m_swapchainImages.size());

		VULKAN_CHECK(vkAllocateDescriptorSets(l_renderDevice.m_device, 
			&allocInfo, m_descriptorSet.data()));

		for (size_t i = 0; i < l_renderDevice.m_swapchainImages.size(); i++)
		{
			VkDescriptorSet ds = m_descriptorSet[i];

			const VkDescriptorBufferInfo bufferInfo = { m_uniformBuffers[i], 0, sizeof(glm::mat4) };
			const VkDescriptorImageInfo  imageInfo = { m_cubeTextureSampler, m_cubeTexture.imageView, 
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

			const std::array<VkWriteDescriptorSet, 2> descriptorWrites = {
				bufferWriteDescriptorSet(ds, &bufferInfo,  0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
				imageWriteDescriptorSet(ds, &imageInfo,   1)
			};

			vkUpdateDescriptorSets(l_renderDevice.m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}

		return true;
	}

	void VulkanCubeRenderer::UpdateStorageBuffers(VulkanRenderDevice& l_renderDevice,
		uint32_t l_currentSwapchainIndex, const glm::mat4& l_mvp)
	{
		memcpy(m_mappedUniformData[l_currentSwapchainIndex], &l_mvp, sizeof(glm::mat4));
	}

	VulkanCubeRenderer::~VulkanCubeRenderer()
	{
		vkDestroySampler(m_device, m_cubeTextureSampler, nullptr);
		destroyVulkanImage(m_device, m_cubeTexture);
	}

	void VulkanCubeRenderer::FillCommandBuffer(VkCommandBuffer l_commandBuffer,
		uint32_t l_currentSwapchainIndex, VkFramebuffer l_frameBuffer = VK_NULL_HANDLE,
		VkRenderPass l_renderpass = VK_NULL_HANDLE)
	{
		BeginRenderPass(l_commandBuffer, l_currentSwapchainIndex);

		vkCmdDraw(l_commandBuffer, 36, 1, 0, 0);

		vkCmdEndRenderPass(l_commandBuffer);
	}
}