



#include "VulkanModelRenderer.hpp"
#include "ErrorCheck.hpp"
#include <glm/glm.hpp>
#include <array>


namespace RenderCore
{

	VulkanModelRenderer::~VulkanModelRenderer()
	{
		vkFreeMemory(m_device, m_storageVertexIndexGpuMemory, nullptr);
		vkFreeMemory(m_device, m_modelTexture.imageMemory, nullptr);

		vkDestroyBuffer(m_device, m_storageVertexIndexBuffer, nullptr);
		vkDestroyImage(m_device, m_modelTexture.image, nullptr);
		vkDestroySampler(m_device, m_textureSampler, nullptr);

		vkDestroyImageView(m_device, m_modelTexture.imageView, nullptr);
	}


	VulkanModelRenderer::VulkanModelRenderer(VulkanRenderDevice& l_renderDevice,
		VulkanImage l_depth,
		const char* l_modelFile,
		const char* l_textureFile,
		uint32_t l_uniformDataSize) : Renderbase(l_renderDevice, l_depth)
	{

		if (false == createTexturedVertexBuffer(l_renderDevice, l_modelFile, &m_storageVertexIndexBuffer,
			&m_storageVertexIndexGpuMemory, &m_vertexSubArraySize, &m_indiceSubArraySize)) {
			printf("Vertex buffer creation failed at file: %s\nAround line number %d", __FILE__, __LINE__);
			exit(EXIT_FAILURE);
		}


		if (false == createTextureImage(l_renderDevice, l_textureFile, m_modelTexture.image,
			m_modelTexture.imageMemory)) {
			printf("Texture image creation failed at file: %s\nAround line number %d", __FILE__, __LINE__);
			exit(EXIT_FAILURE);
		}

		if (false == createImageView(l_renderDevice.m_device, m_modelTexture.image, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_ASPECT_COLOR_BIT, &m_modelTexture.imageView)) {
			printf("Texture image view creation failed at file: %s\nAround line number %d", __FILE__, __LINE__);
			exit(EXIT_FAILURE);
		}

		if (false == createTextureSampler(l_renderDevice.m_device, &m_textureSampler)) {
			printf("Texture sampler creation failed at file: %s\nAround line number %d", __FILE__, __LINE__);
			exit(EXIT_FAILURE);
		}


		if (false == createColorAndDepthRenderPass(l_renderDevice, true, &m_renderPass, RenderPassCreateInfo())) {
			printf("Render pass creation failed at file: %s\nAround line number %d", __FILE__, __LINE__);
			exit(EXIT_FAILURE);
		}

		if (false == createColorAndDepthFramebuffers(l_renderDevice, m_renderPass, m_depth.imageView, m_frameBuffers)) {
			printf("Framebuffer creation failed at file: %s\nAround line number %d", __FILE__, __LINE__);
			exit(EXIT_FAILURE);
		}

		if (false == CreateUniformBuffers(l_renderDevice, l_uniformDataSize)) {
			printf("Uniform buffer creation failed at file : % s\nAround line number % d", __FILE__, __LINE__);
			exit(EXIT_FAILURE);
		}

		if (false == createDescriptorPool(l_renderDevice, 1, 2, 1, &m_descriptorPool)) {
			printf("Descriptor pool creation failed at file: %s\nAround line number %d", __FILE__, __LINE__);
			exit(EXIT_FAILURE);
		}

		if (false == CreateDescriptorSet(l_renderDevice, l_uniformDataSize)) {
			printf("Descriptor set allocation creation failed at file: %s\nAround line number %d", __FILE__, __LINE__);
			exit(EXIT_FAILURE);
		}


		


		if (false == createPipelineLayout(l_renderDevice.m_device, m_descriptorSetLayout, &m_pipelineLayout)) {
			printf("Pipeline layout creation failed at file: %s\nAround line number %d", __FILE__, __LINE__);
			exit(EXIT_FAILURE);
		}

		if (false == createGraphicsPipeline(l_renderDevice, m_renderPass, m_pipelineLayout, 
			{ "data/shaders/chapter03/VK02.vert", "data/shaders/chapter03/VK02.frag", "data/shaders/chapter03/VK02.geom" }, &m_graphicsPipeline)) {

			printf("Graphics pipeline creation failed at file: %s\nAround line number %d", __FILE__, __LINE__);
			exit(EXIT_FAILURE);
		}
	}


	void VulkanModelRenderer::FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
		uint32_t l_currentSwapchainIndex, VkFramebuffer l_frameBuffer = VK_NULL_HANDLE,
		VkRenderPass l_renderpass = VK_NULL_HANDLE)
	{
		BeginRenderPass(l_cmdBuffer, l_currentSwapchainIndex);
		vkCmdDraw(l_cmdBuffer, m_indiceSubArraySize/sizeof(unsigned int), 1, 0, 0);
		vkCmdEndRenderPass(l_cmdBuffer);
	}

	void VulkanModelRenderer::UpdateStorageBuffers(VulkanRenderDevice& l_renderDevice,
		uint32_t l_currentSwapchainIndex,
		const void* l_data, size_t l_dataSize)
	{
		memcpy(m_mappedUniformData[l_currentSwapchainIndex], l_data, l_dataSize);
	}


	bool VulkanModelRenderer::CreateDescriptorSet(VulkanRenderDevice& l_renderDevice,
		uint32_t l_uniformBufferDataSize)
	{

		std::array<VkDescriptorSetLayoutBinding, 4> lv_setLayoutBindings
		{
				descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					VK_SHADER_STAGE_VERTEX_BIT),
				descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ,
					VK_SHADER_STAGE_VERTEX_BIT),
				descriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ,
					VK_SHADER_STAGE_VERTEX_BIT),
				descriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		VkDescriptorSetLayoutCreateInfo lv_setLayoutCreateInfo = {};
		lv_setLayoutCreateInfo.bindingCount = (uint32_t)lv_setLayoutBindings.size();
		lv_setLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		lv_setLayoutCreateInfo.pBindings = lv_setLayoutBindings.data();
		
		VK_CHECK(vkCreateDescriptorSetLayout(l_renderDevice.m_device, &lv_setLayoutCreateInfo,
			nullptr, &m_descriptorSetLayout));

		std::vector<VkDescriptorSetLayout> lv_descriptorSetLayoutsForAlloc{ m_frameBuffers.size(),
		m_descriptorSetLayout };

		VkDescriptorSetAllocateInfo lv_setAllocateInfo = {};
		lv_setAllocateInfo.descriptorPool = m_descriptorPool;
		lv_setAllocateInfo.descriptorSetCount = (uint32_t)m_frameBuffers.size();
		lv_setAllocateInfo.pSetLayouts = lv_descriptorSetLayoutsForAlloc.data();
		lv_setAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

		m_descriptorSet.resize(m_frameBuffers.size());

		
		VK_CHECK(vkAllocateDescriptorSets(l_renderDevice.m_device, &lv_setAllocateInfo, m_descriptorSet.data()));
		

		for (size_t i = 0; i < m_frameBuffers.size(); ++i) {
			
			VkDescriptorBufferInfo lv_bufferInfoUniform = {};
			lv_bufferInfoUniform.buffer = m_uniformBuffers[i];
			lv_bufferInfoUniform.range = l_uniformBufferDataSize;

			VkDescriptorBufferInfo lv_bufferInfoVertex = {};
			lv_bufferInfoVertex.buffer = m_storageVertexIndexBuffer;
			lv_bufferInfoVertex.range = m_vertexSubArraySize;

			VkDescriptorBufferInfo lv_bufferInfoIndex = {};
			lv_bufferInfoIndex.buffer = m_storageVertexIndexBuffer;
			lv_bufferInfoIndex.offset = m_vertexSubArraySize;
			lv_bufferInfoIndex.range = m_indiceSubArraySize;

			VkDescriptorImageInfo lv_imageInfo = {};
			lv_imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfo.imageView = m_modelTexture.imageView;
			lv_imageInfo.sampler = m_textureSampler;

			std::array<VkWriteDescriptorSet, 4> lv_writeDescriptorSets
			{
				bufferWriteDescriptorSet(m_descriptorSet[i], &lv_bufferInfoUniform, 0, 
					VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
				bufferWriteDescriptorSet(m_descriptorSet[i], &lv_bufferInfoVertex, 1,
					VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
				bufferWriteDescriptorSet(m_descriptorSet[i], &lv_bufferInfoIndex, 2,
					VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
				imageWriteDescriptorSet(m_descriptorSet[i], &lv_imageInfo, 3)
			};

			vkUpdateDescriptorSets(l_renderDevice.m_device, lv_writeDescriptorSets.size(),
				lv_writeDescriptorSets.data(), 0, nullptr);

		}

		return true;
	}
}