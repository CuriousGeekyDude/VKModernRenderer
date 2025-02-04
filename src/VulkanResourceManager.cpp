



#include "VulkanResourceManager.hpp"
#include "VulkanEngineCore.hpp"
#include <format>


namespace RenderCore
{

	VulkanResourceManager::VulkanResourceManager(VulkanRenderDevice& l_renderDevice)
		:m_renderDevice(l_renderDevice) {

		auto lv_totalNumSwapchhains = l_renderDevice.m_swapchainImages.size();

		m_buffers.reserve(16);
		m_textures.reserve(256);

		m_frameBuffers.reserve(16);
		m_renderPasses.reserve(16);

		m_descriptorPools.reserve(4);
		m_descriptorSetLayouts.reserve(16);

		m_pipelineLayouts.reserve(16);
		m_Pipelines.reserve(16);

		for (size_t i = 0; i < lv_totalNumSwapchhains; ++i) {

			VulkanTexture lv_swapchain{};
			lv_swapchain.depth = 0;
			lv_swapchain.format = VK_FORMAT_B8G8R8A8_UNORM;
			lv_swapchain.height = l_renderDevice.m_framebufferHeight;
			lv_swapchain.width = l_renderDevice.m_framebufferWidth;
			lv_swapchain.sampler = nullptr;
			lv_swapchain.image.image = l_renderDevice.m_swapchainImages[i];
			lv_swapchain.image.imageView = l_renderDevice.m_swapchainImageViews[i];

			m_textures.push_back(lv_swapchain);


			std::string lv_formattedString{ "Swapchain {}" };
			auto lv_formattedArgs = std::make_format_args(i);

			AddGpuResource(std::vformat(lv_formattedString, lv_formattedArgs).c_str(), i, VulkanDataType::m_texture);
		}

		for (size_t i = 0; i < lv_totalNumSwapchhains; ++i) {

			std::string lv_formattedString{ "Depth {}" };
			auto lv_formattedArgs = std::make_format_args(i);

			auto lv_depthHandle = CreateDepthTextureWithHandle(std::vformat(lv_formattedString, lv_formattedArgs).c_str());

			AddGpuResource(std::vformat(lv_formattedString, lv_formattedArgs).c_str(), lv_depthHandle,VulkanDataType::m_texture);
		}


	}



	void VulkanResourceManager::CopyDataToLocalBuffer(VkQueue l_queue, VkCommandBuffer l_cmdBuffer,
		const uint32_t l_bufferHandle, const void* l_dstBufferData)
	{
		using namespace ErrorCheck;


		auto& lv_buffer = RetrieveGpuBuffer(l_bufferHandle);

		auto& lv_stagingBuffer = CreateBuffer(lv_buffer.size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			, "TemporaryStagingBufferForDataCopy ");


		memcpy(lv_stagingBuffer.ptr, l_dstBufferData, lv_buffer.size);


		VkBufferCopy lv_bufferCopy = {};
		lv_bufferCopy.size = lv_buffer.size;

		VkCommandBufferBeginInfo lv_commandBufferBegin = {};
		lv_commandBufferBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		lv_commandBufferBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VULKAN_CHECK(vkBeginCommandBuffer(l_cmdBuffer, &lv_commandBufferBegin));

		vkCmdCopyBuffer(l_cmdBuffer, lv_stagingBuffer.buffer,
			lv_buffer.buffer,
			1, &lv_bufferCopy);

		vkEndCommandBuffer(l_cmdBuffer);

		VkSubmitInfo lv_submitInfo = {};
		lv_submitInfo.commandBufferCount = 1;
		lv_submitInfo.pCommandBuffers = &l_cmdBuffer;
		lv_submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VULKAN_CHECK(vkQueueSubmit(l_queue, 1, &lv_submitInfo, nullptr));

		vkQueueWaitIdle(l_queue);
	}


	VulkanBuffer& VulkanResourceManager::CreateSharedBuffer(VkDeviceSize l_size, VkBufferUsageFlags l_usage,
		VkMemoryPropertyFlags l_memoryProperties,
		const char* l_nameBuffer)
	{
		using namespace ErrorCheck;

		VulkanBuffer lv_bufferToCreate{};
		
		if (false == createSharedBuffer(m_renderDevice, l_size,
			l_usage, l_memoryProperties, lv_bufferToCreate.buffer, lv_bufferToCreate.memory)) {
			PRINT_EXIT(".\nFailed to create shared vulkan buffer memory.\n");
		}

		lv_bufferToCreate.size = l_size;

		if (true == (0 != (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & l_memoryProperties))
			&&
			(0 != (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & l_memoryProperties))) {
			VULKAN_CHECK(vkMapMemory(m_renderDevice.m_device,
				lv_bufferToCreate.memory, 0, l_size, 0, &lv_bufferToCreate.ptr));
		}


		m_buffers.push_back(lv_bufferToCreate);

		VkDebugUtilsObjectNameInfoEXT lv_objectNameInfo{};
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_bufferToCreate.buffer);
		lv_objectNameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
		lv_objectNameInfo.pNext = nullptr;
		lv_objectNameInfo.pObjectName = l_nameBuffer;
		lv_objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;

		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));

		return m_buffers[m_buffers.size()-1];
	}






	VkPipeline VulkanResourceManager::CreateComputePipeline(VkDevice m_device, const char* l_computeShaderFilePath
		,VkPipelineLayout pipelineLayout)
	{
		using namespace ErrorCheck;

		VkPipeline lv_computePipeline;

		ShaderModule lv_computeShaderModule;
		VULKAN_CHECK(createShaderModule(m_device, &lv_computeShaderModule , l_computeShaderFilePath));

		VULKAN_CHECK(createComputePipeline(m_device, lv_computeShaderModule.shaderModule, pipelineLayout,&lv_computePipeline));

		m_Pipelines.push_back(lv_computePipeline);

		return lv_computePipeline;
	}




	VulkanBuffer& VulkanResourceManager::CreateBuffer(VkDeviceSize l_size, VkBufferUsageFlags l_usage,
		VkMemoryPropertyFlags l_memoryProperties, const char* l_nameBuffer)
	{
		using namespace ErrorCheck;

		VulkanBuffer lv_bufferToCreate{};

		if (false == createBuffer(m_renderDevice.m_device, m_renderDevice.m_physicalDevice, l_size,
			l_usage, l_memoryProperties, lv_bufferToCreate.buffer, lv_bufferToCreate.memory)) {
			PRINT_EXIT(".\nFailed to create vulkan buffer.\n");
		}

		lv_bufferToCreate.size = l_size;

		if (true == (0 != (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & l_memoryProperties))
			&&
			(0 != (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & l_memoryProperties))) {
			VULKAN_CHECK(vkMapMemory(m_renderDevice.m_device,
				lv_bufferToCreate.memory, 0, l_size, 0, &lv_bufferToCreate.ptr));
		}


		m_buffers.push_back(lv_bufferToCreate);


		VkDebugUtilsObjectNameInfoEXT lv_objectNameInfo{};
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_bufferToCreate.buffer);
		lv_objectNameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
		lv_objectNameInfo.pNext = nullptr;
		lv_objectNameInfo.pObjectName = l_nameBuffer;
		lv_objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;

		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device,&lv_objectNameInfo));


		return m_buffers[m_buffers.size()-1];
	}




	VulkanTexture& 
		VulkanResourceManager::CreateTextureForOffscreenFrameBuffer(const std::string& l_nameTexture,
		VkFormat l_colorFormat, 
		VkFilter l_minFilter,
		VkFilter l_maxFilter,
		VkSamplerAddressMode l_addressMode)
	{
		using namespace ErrorCheck;

		VulkanTexture lv_textureToCreate{};
		lv_textureToCreate.format = l_colorFormat;
		lv_textureToCreate.height = m_renderDevice.m_framebufferHeight;
		lv_textureToCreate.width = m_renderDevice.m_framebufferWidth;
		lv_textureToCreate.depth = 1U;

		if (false == createOffscreenImage(m_renderDevice, lv_textureToCreate.image.image,
			lv_textureToCreate.image.imageMemory, lv_textureToCreate.width, lv_textureToCreate.height,
			l_colorFormat, 1, 0)) {
			PRINT_EXIT(".\nFailed to create attachment for an offscreen framebuffer.\n");
		}

		VkDebugUtilsObjectNameInfoEXT lv_objectNameInfo{};
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_textureToCreate.image.image);
		lv_objectNameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
		lv_objectNameInfo.pNext = nullptr;
		lv_objectNameInfo.pObjectName = l_nameTexture.c_str();
		lv_objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;


		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));


		if (false == createImageView(m_renderDevice.m_device, lv_textureToCreate.image.image, 
			l_colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, &lv_textureToCreate.image.imageView)) {
			PRINT_EXIT(".\nFailed to create image view of an offscreen attachment.\n");
		}


		std::string lv_imageViewName{ l_nameTexture + "-view " };
		VkDebugUtilsObjectNameInfoEXT lv_objectNameInfo1{};
		lv_objectNameInfo1.objectHandle = reinterpret_cast<uint64_t>(lv_textureToCreate.image.imageView);
		lv_objectNameInfo1.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
		lv_objectNameInfo1.pNext = nullptr;
		lv_objectNameInfo1.pObjectName = lv_imageViewName.c_str();
		lv_objectNameInfo1.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;


		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo1));

		if (false == createTextureSampler(m_renderDevice.m_device, &lv_textureToCreate.sampler, l_minFilter,
			l_maxFilter, l_addressMode)) {
			PRINT_EXIT(".\nFailed to create sampler for offscreen attachment.\n");
		}


		std::string lv_samplerName{ l_nameTexture + "-sampler " };
		VkDebugUtilsObjectNameInfoEXT lv_objectNameInfo2{};
		lv_objectNameInfo2.objectHandle = reinterpret_cast<uint64_t>(lv_textureToCreate.sampler);
		lv_objectNameInfo2.objectType = VK_OBJECT_TYPE_SAMPLER;
		lv_objectNameInfo2.pNext = nullptr;
		lv_objectNameInfo2.pObjectName = lv_samplerName.c_str();
		lv_objectNameInfo2.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;



		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo2));



		transitionImageLayout(m_renderDevice, lv_textureToCreate.image.image, l_colorFormat, VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		lv_textureToCreate.Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		m_textures.push_back(lv_textureToCreate);




		return m_textures[m_textures.size() - 1];

	}


	VulkanTexture& VulkanResourceManager::LoadTexture2D(const std::string& l_textureFileName)
	{
		
		using namespace ErrorCheck;

		VulkanTexture lv_textureToCreate{};


		printf("\nAttempting to load : %s\n", l_textureFileName.c_str());

		if (false == createTextureImage(m_renderDevice, l_textureFileName.c_str(),
			lv_textureToCreate.image.image, lv_textureToCreate.image.imageMemory,
			&lv_textureToCreate.width, &lv_textureToCreate.height)) {

			PRINT_EXIT("\nFailed to create texture image from texture file name.\n");

		}

		if (false == createImageView(m_renderDevice.m_device, lv_textureToCreate.image.image,
			VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &lv_textureToCreate.image.imageView)) {
			PRINT_EXIT("\nFailed to create image view for the loaded 2D texture file.\n");
		}

		if (false == createTextureSampler(m_renderDevice.m_device, &lv_textureToCreate.sampler)) {
			PRINT_EXIT("\nFailed to create sampler for loaded 2D texture.\n");
		}

		transitionImageLayout(m_renderDevice, lv_textureToCreate.image.image, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


		lv_textureToCreate.format = VK_FORMAT_R8G8B8A8_UNORM;
		lv_textureToCreate.depth = 1U;
		lv_textureToCreate.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		m_textures.push_back(lv_textureToCreate);

		return m_textures[m_textures.size() - 1];
	}


	uint32_t VulkanResourceManager::LoadTexture2DWithHandle(const std::string& l_textureFileName)
	{
		LoadTexture2D(l_textureFileName);

		return (uint32_t)m_textures.size() - 1;
	}


	uint32_t VulkanResourceManager::CreateBufferWithHandle(VkDeviceSize l_size, VkBufferUsageFlags l_usage,
		VkMemoryPropertyFlags l_memoryProperties, const char* l_nameBuffer)
	{
		CreateBuffer(l_size, l_usage, l_memoryProperties, l_nameBuffer);

		return (uint32_t)m_buffers.size() - 1;
	}



	VulkanTexture& VulkanResourceManager::CreateDepthTextureForOffscreenFrameBuffer(const std::string& l_nameDepthTexture)
	{
		using namespace ErrorCheck;

		VulkanTexture lv_depthTextureToCreate{};
		lv_depthTextureToCreate.height = m_renderDevice.m_framebufferHeight;
		lv_depthTextureToCreate.width = m_renderDevice.m_framebufferWidth;
		lv_depthTextureToCreate.format = findDepthFormat(m_renderDevice.m_physicalDevice);
		lv_depthTextureToCreate.depth = 1U;


		if (false == createImage(m_renderDevice.m_device, m_renderDevice.m_physicalDevice, lv_depthTextureToCreate.width,
			lv_depthTextureToCreate.height, lv_depthTextureToCreate.format, VK_IMAGE_TILING_OPTIMAL, 
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			lv_depthTextureToCreate.image.image, lv_depthTextureToCreate.image.imageMemory)) {
			PRINT_EXIT("\nFailed to create depth texture.\n");
		}


		VkDebugUtilsObjectNameInfoEXT lv_objectNameInfo{};
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_depthTextureToCreate.image.image);
		lv_objectNameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
		lv_objectNameInfo.pNext = nullptr;
		lv_objectNameInfo.pObjectName = l_nameDepthTexture.c_str();
		lv_objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;

		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));


		if (false == createImageView(m_renderDevice.m_device, lv_depthTextureToCreate.image.image,
			lv_depthTextureToCreate.format, VK_IMAGE_ASPECT_DEPTH_BIT, &lv_depthTextureToCreate.image.imageView)) {
			PRINT_EXIT("\nFaled to create image view of depth texture\n");
		}


		std::string lv_imageViewName{ l_nameDepthTexture + "-view " };
		VkDebugUtilsObjectNameInfoEXT lv_objectNameInfo1{};
		lv_objectNameInfo1.objectHandle = reinterpret_cast<uint64_t>(lv_depthTextureToCreate.image.imageView);
		lv_objectNameInfo1.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
		lv_objectNameInfo1.pNext = nullptr;
		lv_objectNameInfo1.pObjectName = lv_imageViewName.c_str();
		lv_objectNameInfo1.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;

		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo1));



		if (false == createTextureSampler(m_renderDevice.m_device, &lv_depthTextureToCreate.sampler)) {
			PRINT_EXIT("\nFailed to create image sampler for depth texture.\n");
		}

		std::string lv_samplerName{ l_nameDepthTexture + "-sampler " };
		VkDebugUtilsObjectNameInfoEXT lv_objectNameInfo2{};
		lv_objectNameInfo2.objectHandle = reinterpret_cast<uint64_t>(lv_depthTextureToCreate.sampler);
		lv_objectNameInfo2.objectType = VK_OBJECT_TYPE_SAMPLER;
		lv_objectNameInfo2.pNext = nullptr;
		lv_objectNameInfo2.pObjectName = lv_samplerName.c_str();
		lv_objectNameInfo2.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;

		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo2));



		transitionImageLayout(m_renderDevice, lv_depthTextureToCreate.image.image, lv_depthTextureToCreate.format,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		m_textures.push_back(lv_depthTextureToCreate);

		return m_textures[m_textures.size() - 1];
	}



	uint32_t VulkanResourceManager::CreateTexture(const char* l_nameTexture,
		VkFormat l_colorFormat,
		VkFilter l_minFilter,
		VkFilter l_maxFilter,
		VkSamplerAddressMode l_addressMode)
	{
		CreateTextureForOffscreenFrameBuffer(l_nameTexture, l_colorFormat, l_minFilter, l_maxFilter, l_addressMode);

		return (uint32_t)m_textures.size()-1;
	}


	VulkanBuffer& VulkanResourceManager::RetrieveGpuBuffer(const uint32_t l_handle)
	{
		return m_buffers.at(l_handle);
	}
	VulkanTexture& VulkanResourceManager::RetrieveGpuTexture(const uint32_t l_handle)
	{
		return m_textures.at(l_handle);
	}
	VkFramebuffer VulkanResourceManager::RetrieveGpuFramebuffer(const uint32_t l_handle)
	{
		return m_frameBuffers.at(l_handle);
	}
	VkRenderPass VulkanResourceManager::RetrieveGpuRenderpass(const uint32_t l_handle)
	{
		return m_renderPasses.at(l_handle);
	}
	VkPipelineLayout VulkanResourceManager::RetrieveGpuPipelineLayout(const uint32_t l_handle)
	{
		return m_pipelineLayouts.at(l_handle);
	}
	VkPipeline	VulkanResourceManager::RetrieveGpuPipeline(const uint32_t l_handle)
	{
		return m_Pipelines.at(l_handle);
	}
	VkDescriptorSetLayout VulkanResourceManager::RetrieveGpuDescriptorSetLayout(const uint32_t l_handle)
	{
		return m_descriptorSetLayouts.at(l_handle);
	}
	VkDescriptorPool VulkanResourceManager::RetrieveGpuDescriptorPool(const uint32_t l_handle)
	{
		return m_descriptorPools.at(l_handle);
	}

	VulkanTexture& VulkanResourceManager::CreateDepthTexture(const std::string& l_nameDepthTexture)
	{
		using namespace ErrorCheck;

		VulkanTexture lv_depthTextureToCreate{};
		lv_depthTextureToCreate.height = m_renderDevice.m_framebufferHeight;
		lv_depthTextureToCreate.width = m_renderDevice.m_framebufferWidth;
		lv_depthTextureToCreate.format = findDepthFormat(m_renderDevice.m_physicalDevice);
		lv_depthTextureToCreate.depth = 1U;


		if (false == createImage(m_renderDevice.m_device, m_renderDevice.m_physicalDevice, lv_depthTextureToCreate.width,
			lv_depthTextureToCreate.height, lv_depthTextureToCreate.format, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			lv_depthTextureToCreate.image.image, lv_depthTextureToCreate.image.imageMemory)) {
			PRINT_EXIT("\nFailed to create depth texture.\n");
		}


		VkDebugUtilsObjectNameInfoEXT lv_objectNameInfo{};
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_depthTextureToCreate.image.image);
		lv_objectNameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
		lv_objectNameInfo.pNext = nullptr;
		lv_objectNameInfo.pObjectName = l_nameDepthTexture.c_str();
		lv_objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;


		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));


		if (false == createImageView(m_renderDevice.m_device, lv_depthTextureToCreate.image.image,
			lv_depthTextureToCreate.format, VK_IMAGE_ASPECT_DEPTH_BIT, &lv_depthTextureToCreate.image.imageView)) {
			PRINT_EXIT("\nFaled to create image view of depth texture\n");
		}


		std::string lv_imageViewName{ l_nameDepthTexture + "-view "};
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_depthTextureToCreate.image.imageView);
		lv_objectNameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
		lv_objectNameInfo.pNext = nullptr;
		lv_objectNameInfo.pObjectName = lv_imageViewName.c_str();
		lv_objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;


		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));


		if (false == createTextureSampler(m_renderDevice.m_device, &lv_depthTextureToCreate.sampler)) {
			PRINT_EXIT("\nFailed to create image sampler for depth texture.\n");
		}


		std::string lv_samplerName{ l_nameDepthTexture + "-sampler " };
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_depthTextureToCreate.sampler);
		lv_objectNameInfo.objectType = VK_OBJECT_TYPE_SAMPLER;
		lv_objectNameInfo.pNext = nullptr;
		lv_objectNameInfo.pObjectName = lv_samplerName.c_str();
		lv_objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;


		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));


		transitionImageLayout(m_renderDevice, lv_depthTextureToCreate.image.image, lv_depthTextureToCreate.format,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		lv_depthTextureToCreate.Layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		m_textures.push_back(lv_depthTextureToCreate);

		return m_textures[m_textures.size() - 1];
	}




	uint32_t VulkanResourceManager::CreateDepthTextureWithHandle(const std::string& l_nameTexture)
	{
		CreateDepthTexture(l_nameTexture);

		return (uint32_t)m_textures.size() - 1;
	}




	VkFramebuffer& VulkanResourceManager::CreateFrameBuffer(const RenderPass& l_renderpass, 
		const std::vector<VulkanTexture>& l_images, const char* l_nameFrameBuffer)
	{
		using namespace ErrorCheck;

		VkFramebuffer lv_frameBufferToCreate{};


		std::vector<VkImageView> lv_attachments{};

		if (0 == l_images.size()) {
			PRINT_EXIT("\nl_images parameter is empty. Framebuffer creation failed.\n");
		}

		for (auto& l_image : l_images) {
			lv_attachments.push_back(l_image.image.imageView);
		}

		VkFramebufferCreateInfo lv_frameBufferCreateInfo{};
		lv_frameBufferCreateInfo.attachmentCount = (uint32_t)lv_attachments.size();
		lv_frameBufferCreateInfo.height = l_images[0].height;
		lv_frameBufferCreateInfo.width = l_images[0].width;
		lv_frameBufferCreateInfo.pAttachments = lv_attachments.data();
		lv_frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		lv_frameBufferCreateInfo.renderPass = l_renderpass.m_renderpass;
		lv_frameBufferCreateInfo.layers = 1U;

		VULKAN_CHECK(vkCreateFramebuffer(m_renderDevice.m_device, &lv_frameBufferCreateInfo, nullptr, &lv_frameBufferToCreate));

		m_frameBuffers.push_back(lv_frameBufferToCreate);


		VkDebugUtilsObjectNameInfoEXT lv_objectNameInfo{};
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_frameBufferToCreate);
		lv_objectNameInfo.objectType = VK_OBJECT_TYPE_FRAMEBUFFER;
		lv_objectNameInfo.pNext = nullptr;
		lv_objectNameInfo.pObjectName = l_nameFrameBuffer;
		lv_objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;


		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));



		return m_frameBuffers[m_frameBuffers.size() - 1];
	}







	void VulkanResourceManager::AddGpuResource(const char* l_nameResource, 
		uint32_t l_resourceHandle, VulkanDataType l_vkDataType)
	{
		std::string lv_nameResource{ l_nameResource };
		GpuResourceMetaData lv_metaData{.m_vkDataType = l_vkDataType, .m_resourceHandle = l_resourceHandle};
		m_gpuResourcesHandles.insert(std::make_pair<std::string, GpuResourceMetaData>
			(std::move(lv_nameResource), std::move(lv_metaData)));
	}


	VulkanResourceManager::GpuResourceMetaData 
		VulkanResourceManager::RetrieveGpuResourceMetaData(const std::string& l_nameResource)
	{		
		GpuResourceMetaData lv_resourceMetaData{};
		lv_resourceMetaData.m_resourceHandle = UINT32_MAX;
		lv_resourceMetaData.m_vkDataType = VulkanDataType::m_invalid;

		if (auto lv_result = m_gpuResourcesHandles.find(l_nameResource);
			m_gpuResourcesHandles.cend() != lv_result) {

			return lv_result->second;

		}

		return lv_resourceMetaData;
	}

	void* VulkanResourceManager::RetrieveGpuResource(const GpuResourceMetaData& l_resourceMetaData)
	{
		auto lv_vkDataType = l_resourceMetaData.m_vkDataType;

		switch (lv_vkDataType) {

			case VulkanDataType::m_invalid:
				return nullptr;
			case VulkanDataType::m_buffer:
				return (void*)&m_buffers[l_resourceMetaData.m_resourceHandle];
			case VulkanDataType::m_texture:
				return (void*)&m_textures[l_resourceMetaData.m_resourceHandle];
			case VulkanDataType::m_framebuffer:
				return (void*)&m_frameBuffers[l_resourceMetaData.m_resourceHandle];
			case VulkanDataType::m_renderpass:
				return (void*)&m_renderPasses[l_resourceMetaData.m_resourceHandle];
			case VulkanDataType::m_pipelineLayout:
				return (void*)&m_pipelineLayouts[l_resourceMetaData.m_resourceHandle];
			case VulkanDataType::m_pipeline:
				return (void*)&m_Pipelines[l_resourceMetaData.m_resourceHandle];
			case VulkanDataType::m_descriptorPool:
				return (void*)&m_descriptorPools[l_resourceMetaData.m_resourceHandle];
			case VulkanDataType::m_descriptorSetLayout:
				return (void*)&m_descriptorSetLayouts[l_resourceMetaData.m_resourceHandle];

		}

		return nullptr;
	}


	uint32_t VulkanResourceManager::AddVulkanBuffer(const VulkanBuffer& l_buffer)
	{
		m_buffers.push_back(l_buffer);
		return (uint32_t)m_buffers.size()-1;
	}
	uint32_t VulkanResourceManager::AddVulkanTexture(const VulkanTexture& l_texture)
	{
		m_textures.push_back(l_texture);
		return (uint32_t)m_textures.size()-1;
	}
	uint32_t VulkanResourceManager::AddVulkanFramebuffer(VkFramebuffer l_framebuffer)
	{
		m_frameBuffers.push_back(l_framebuffer);
		return (uint32_t)m_frameBuffers.size()-1;
	}
	uint32_t VulkanResourceManager::AddVulkanRenderpass(VkRenderPass l_renderpass)
	{
		m_renderPasses.push_back(l_renderpass);
		return(uint32_t)m_renderPasses.size()-1;
	}
	uint32_t VulkanResourceManager::AddVulkanPipelineLayout(VkPipelineLayout l_pipelineLayout)
	{
		m_pipelineLayouts.push_back(l_pipelineLayout);
		return(uint32_t)m_pipelineLayouts.size()-1;
	}
	uint32_t VulkanResourceManager::AddVulkanPipeline(VkPipeline l_pipeline)
	{
		m_Pipelines.push_back(l_pipeline);
		return(uint32_t)m_Pipelines.size()-1;
	}
	uint32_t VulkanResourceManager::AddVulkanDescriptorSetLayout(VkDescriptorSetLayout l_dsSetLayout)
	{
		m_descriptorSetLayouts.push_back(l_dsSetLayout);
		return(uint32_t)m_descriptorSetLayouts.size()-1;
	}
	uint32_t VulkanResourceManager::AddVulkanDescriptorPool(VkDescriptorPool l_dsPool)
	{
		m_descriptorPools.push_back(l_dsPool);
		return(uint32_t)m_descriptorPools.size()-1;
	}


	VulkanResourceManager::RenderPass VulkanResourceManager::CreateRenderPass(const std::vector<VulkanTexture>& l_textures,
		const char* l_nameRenderpass,
		const RenderPassCreateInfo l_ci,
		bool l_useDepth)
	{
		using namespace ErrorCheck;

		VulkanResourceManager::RenderPass lv_renderpassToCreate{};

		if (0 == l_textures.size()) {
			PRINT_EXIT("\nNumber of textures are zero. Failed to create renderpass.\n");
		}

		if (2 < l_textures.size()) {
			PRINT_EXIT("\nNumber of textures exceeds 2. Failed to create renderpass.\n");
		}

		if (2 == l_textures.size()) {
			if (false == createColorAndDepthRenderPass(m_renderDevice, l_useDepth,
				&lv_renderpassToCreate.m_renderpass, l_ci)) {
				PRINT_EXIT("\nFailed to create color and depth renderpass.\n");
			}			
		}
		else {
			if (false == createDepthOnlyRenderPass(m_renderDevice,
				&lv_renderpassToCreate.m_renderpass, l_ci)) {
				PRINT_EXIT("\nFailed to create color and depth renderpass.\n");
			}
		}

		lv_renderpassToCreate.m_info = l_ci;
		m_renderPasses.push_back(lv_renderpassToCreate.m_renderpass);


		VkDebugUtilsObjectNameInfoEXT lv_objectNameInfo{};
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_renderpassToCreate.m_renderpass);
		lv_objectNameInfo.objectType = VK_OBJECT_TYPE_RENDER_PASS;
		lv_objectNameInfo.pNext = nullptr;
		lv_objectNameInfo.pObjectName = l_nameRenderpass;
		lv_objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;


		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));


		return lv_renderpassToCreate;
	}

	VulkanResourceManager::RenderPass VulkanResourceManager::CreateDepthOnlyRenderPass(
		const std::vector<VulkanTexture>& l_textures,
		const char* l_nameRenderPass,
		const RenderPassCreateInfo l_ci)
	{
		using namespace ErrorCheck;

		VulkanResourceManager::RenderPass lv_renderpassToCreate{};

		if (0 == l_textures.size()) {
			PRINT_EXIT("\nNumber of textures is zero. Failed to create depth only renderpass.\n");
		}

		if (2 <= l_textures.size()) {
			PRINT_EXIT("\nNumber of textures exceeds 1. Failed to create depth only renderpass.\n");
		}

		if (1 == l_textures.size()) {
			if (false == createDepthOnlyRenderPass(m_renderDevice, &lv_renderpassToCreate.m_renderpass, l_ci)) {
				PRINT_EXIT("\nFailed to create depth only renderpass.\n");
			}
		}
		
		lv_renderpassToCreate.m_info = l_ci;
		m_renderPasses.push_back(lv_renderpassToCreate.m_renderpass);

		VkDebugUtilsObjectNameInfoEXT lv_objectNameInfo{};
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_renderpassToCreate.m_renderpass);
		lv_objectNameInfo.objectType = VK_OBJECT_TYPE_RENDER_PASS;
		lv_objectNameInfo.pNext = nullptr;
		lv_objectNameInfo.pObjectName = l_nameRenderPass;
		lv_objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;


		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));


		return lv_renderpassToCreate;

	}

	VulkanResourceManager::RenderPass VulkanResourceManager::CreateFullScreenRenderPass(bool l_useDepth, 
		const RenderPassCreateInfo& l_ci, const char* l_nameRenderpass)
	{
		using namespace ErrorCheck;

		VulkanResourceManager::RenderPass lv_renderpassToCreate{};
		lv_renderpassToCreate.m_info = l_ci;
		
		if (false == createColorAndDepthRenderPass(m_renderDevice, l_useDepth, &lv_renderpassToCreate.m_renderpass,
			l_ci)) {
			PRINT_EXIT("\nFailed to full screen renderpass.\n");
		}

		m_renderPasses.push_back(lv_renderpassToCreate.m_renderpass);


		VkDebugUtilsObjectNameInfoEXT lv_objectNameInfo{};
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_renderpassToCreate.m_renderpass);
		lv_objectNameInfo.objectType = VK_OBJECT_TYPE_RENDER_PASS;
		lv_objectNameInfo.pNext = nullptr;
		lv_objectNameInfo.pObjectName = l_nameRenderpass;
		lv_objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;


		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));

		return lv_renderpassToCreate;
	}

	std::vector<VkFramebuffer> 
		VulkanResourceManager::CreateFullScreenFrameBuffers(
			VulkanResourceManager::RenderPass l_renderPass, VkImageView l_depthView)
	{
		using namespace ErrorCheck;

		std::vector<VkFramebuffer> lv_fullScreenFrameBuffers{};


		

		if (false == createColorAndDepthFramebuffers(m_renderDevice, l_renderPass.m_renderpass, l_depthView,
			lv_fullScreenFrameBuffers)) {
			PRINT_EXIT("\nFailed to create full screen framebuffers.\n");
		}
		
		

		for (auto l_frameBuffer : lv_fullScreenFrameBuffers) {
			m_frameBuffers.push_back(l_frameBuffer);
		}

		return lv_fullScreenFrameBuffers;
	}


	VkPipelineLayout& VulkanResourceManager::CreatePipelineLayoutWithPush(VkDescriptorSetLayout l_descriptorSetLayout,
		const char* l_namePipelineLayout,
		uint32_t l_vtxConstSize, uint32_t l_fragConstSize)
	{
		using namespace ErrorCheck;

		VkPipelineLayout lv_pipelineLayout;
		if (!createPipelineLayoutWithConstants(m_renderDevice.m_device, l_descriptorSetLayout,
			&lv_pipelineLayout, l_vtxConstSize, l_fragConstSize))
		{
			PRINT_EXIT("\nCannot create pipeline layout with push constants.\n");
		}


		m_pipelineLayouts.push_back(lv_pipelineLayout);

		VkDebugUtilsObjectNameInfoEXT lv_objectNameInfo{};
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_pipelineLayout);
		lv_objectNameInfo.objectType = VK_OBJECT_TYPE_PIPELINE_LAYOUT;
		lv_objectNameInfo.pNext = nullptr;
		lv_objectNameInfo.pObjectName = l_namePipelineLayout;
		lv_objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;


		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));


		return m_pipelineLayouts[m_pipelineLayouts.size() - 1];
	}


	VkPipelineLayout& VulkanResourceManager::CreatePipelineLayout(VkDescriptorSetLayout l_descriptorSetLayout,
		const char* l_namePipelineLayout)
	{
		using namespace ErrorCheck;

		VkPipelineLayout lv_pipelineLayout;
		if (!createPipelineLayout(m_renderDevice.m_device, l_descriptorSetLayout, &lv_pipelineLayout))
		{
			PRINT_EXIT("\nCannot create pipeline layout.\n");
		}

		m_pipelineLayouts.push_back(lv_pipelineLayout);

		VkDebugUtilsObjectNameInfoEXT lv_objectNameInfo{};
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_pipelineLayout);
		lv_objectNameInfo.objectType = VK_OBJECT_TYPE_PIPELINE_LAYOUT;
		lv_objectNameInfo.pNext = nullptr;
		lv_objectNameInfo.pObjectName = l_namePipelineLayout;
		lv_objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;


		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));


		return m_pipelineLayouts[m_pipelineLayouts.size() - 1];
	}


	VkPipeline& VulkanResourceManager::CreateGraphicsPipeline(VkRenderPass l_renderPass, 
		VkPipelineLayout l_pipelineLayout,
		const std::vector<const char*>& l_shaderFiles,
		const char* l_namePipeline,
		const PipelineInfo& l_pipelineParams)
	{
		using namespace ErrorCheck;

		VkPipeline lv_graphicsPipeline{};

		if (false == createGraphicsPipeline(m_renderDevice, l_renderPass, l_pipelineLayout,
			l_shaderFiles, &lv_graphicsPipeline, l_pipelineParams.m_totalNumColorAttach, l_pipelineParams.m_topology, l_pipelineParams.m_useDepth,
			l_pipelineParams.m_useBlending, l_pipelineParams.m_dynamicScissorState, l_pipelineParams.m_width,
			l_pipelineParams.m_height)) {
			PRINT_EXIT("\nFailed to create graphics pipeline.\n");
		}

		m_Pipelines.push_back(lv_graphicsPipeline);


		VkDebugUtilsObjectNameInfoEXT lv_objectNameInfo{};
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_graphicsPipeline);
		lv_objectNameInfo.objectType = VK_OBJECT_TYPE_PIPELINE;
		lv_objectNameInfo.pNext = nullptr;
		lv_objectNameInfo.pObjectName = l_namePipeline;
		lv_objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;


		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));


		return m_Pipelines[m_Pipelines.size() - 1];

	}



	uint32_t VulkanResourceManager::CreateFrameBuffer(const RenderPass& l_renderpass,
		const std::vector<uint32_t>& l_textureHandles,
		const char* l_nameFramebuffer)
	{

		std::vector<VulkanTexture> lv_textures{};
		lv_textures.resize(l_textureHandles.size());

		for (uint32_t i = 0; i < l_textureHandles.size(); ++i) {
			lv_textures[i] = RetrieveGpuTexture(l_textureHandles[i]);
		}

		CreateFrameBuffer(l_renderpass, lv_textures, l_nameFramebuffer);

		return (uint32_t)m_frameBuffers.size() - 1;
	}


	VkDescriptorSetLayout& VulkanResourceManager::CreateDescriptorSetLayout(
		const DescriptorSetResources& l_dsResources, const char* l_nameDsSetLayout)
	{
		using namespace ErrorCheck;

		VkDescriptorSetLayout lv_dsLayout{};
		std::vector<VkDescriptorBindingFlagsEXT> lv_dsBindingFlagsEXT{};
		std::vector<VkDescriptorSetLayoutBinding> lv_bindings{};

		uint32_t lv_bindingNumber{};

		for (auto& l_buffer : l_dsResources.m_buffers) {
			lv_dsBindingFlagsEXT.push_back(0U);
			lv_bindings.push_back(descriptorSetLayoutBinding(lv_bindingNumber++, l_buffer.m_descriptorInfo.m_type,
				l_buffer.m_descriptorInfo.m_shaderStageFlags));
		}

		for (auto& l_texture : l_dsResources.m_textures) {
			lv_dsBindingFlagsEXT.push_back(0U);
			lv_bindings.push_back(descriptorSetLayoutBinding(lv_bindingNumber++, l_texture.m_descriptorInfo.m_type,
				l_texture.m_descriptorInfo.m_shaderStageFlags));
		}

		for (auto& l_textureArray : l_dsResources.m_textureArrays) {
			lv_bindings.push_back(descriptorSetLayoutBinding(lv_bindingNumber++, l_textureArray.m_descriptorInfo.m_type,
				l_textureArray.m_descriptorInfo.m_shaderStageFlags, (uint32_t)l_textureArray.m_textures.size()));
		}

		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT lv_dsBindingFlagsCreateInfoEXT{};
		lv_dsBindingFlagsCreateInfoEXT.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
		lv_dsBindingFlagsCreateInfoEXT.pBindingFlags = lv_dsBindingFlagsEXT.data();
		lv_dsBindingFlagsCreateInfoEXT.bindingCount = (uint32_t)lv_dsBindingFlagsEXT.size();

		VkDescriptorSetLayoutCreateInfo lv_dsLayoutCreateInfo{};
		lv_dsLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		lv_dsLayoutCreateInfo.bindingCount = (uint32_t)lv_bindings.size();
		lv_dsLayoutCreateInfo.pBindings = lv_bindings.data();
		lv_dsLayoutCreateInfo.pNext = &lv_dsBindingFlagsCreateInfoEXT;
		
		VULKAN_CHECK(vkCreateDescriptorSetLayout(m_renderDevice.m_device, &lv_dsLayoutCreateInfo, nullptr,&lv_dsLayout));

		m_descriptorSetLayouts.push_back(lv_dsLayout);

		VkDebugUtilsObjectNameInfoEXT lv_objectNameInfo{};
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_dsLayout);
		lv_objectNameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
		lv_objectNameInfo.pNext = nullptr;
		lv_objectNameInfo.pObjectName = l_nameDsSetLayout;
		lv_objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;


		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));

		
		return m_descriptorSetLayouts[m_descriptorSetLayouts.size() - 1];
	}


	VkDescriptorPool& VulkanResourceManager::CreateDescriptorPool(const DescriptorSetResources& l_dsResources,
		uint32_t l_totalNumDescriptorSets, const char* l_dsPool)
	{

		using namespace ErrorCheck;

		VkDescriptorPool lv_dsPool{};

		std::vector<VkDescriptorPoolSize> lv_dsPoolSizes{};

		uint32_t lv_totalNumUniformBufferArrays{};
		uint32_t lv_totalNumStorageArrays{};
		uint32_t lv_totalNumTextures{};

		
		lv_totalNumTextures = (uint32_t)l_dsResources.m_textures.size();

		for (auto& l_textureArray : l_dsResources.m_textureArrays) {
			lv_totalNumTextures += (uint32_t)l_textureArray.m_textures.size();
		}
		
		for (auto& l_buffer : l_dsResources.m_buffers) {
			if (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER == l_buffer.m_descriptorInfo.m_type) {
				++lv_totalNumUniformBufferArrays;
				continue;
			}
			if (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER == l_buffer.m_descriptorInfo.m_type) {
				++lv_totalNumStorageArrays;
			}
		}

		if (0 != lv_totalNumUniformBufferArrays) {
			lv_dsPoolSizes.push_back(VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.descriptorCount = l_totalNumDescriptorSets*lv_totalNumUniformBufferArrays});
		}

		if (0 != lv_totalNumStorageArrays) {
			lv_dsPoolSizes.push_back(VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = l_totalNumDescriptorSets*lv_totalNumStorageArrays});
		}

		if (0 != lv_totalNumTextures) {
			lv_dsPoolSizes.push_back(VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = lv_totalNumTextures * l_totalNumDescriptorSets });
		}

		VkDescriptorPoolCreateInfo lv_dsPoolCreateInfo{};
		lv_dsPoolCreateInfo.maxSets = l_totalNumDescriptorSets;
		lv_dsPoolCreateInfo.poolSizeCount = (uint32_t)lv_dsPoolSizes.size();
		lv_dsPoolCreateInfo.pPoolSizes = lv_dsPoolSizes.data();
		lv_dsPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		
		
		VULKAN_CHECK(vkCreateDescriptorPool(m_renderDevice.m_device, &lv_dsPoolCreateInfo, nullptr, &lv_dsPool));

		m_descriptorPools.push_back(lv_dsPool);


		VkDebugUtilsObjectNameInfoEXT lv_objectNameInfo{};
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_dsPool);
		lv_objectNameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_POOL;
		lv_objectNameInfo.pNext = nullptr;
		lv_objectNameInfo.pObjectName = l_dsPool;
		lv_objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;


		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));


		return m_descriptorPools[m_descriptorPools.size() - 1];
 	}


	VkDescriptorSet VulkanResourceManager::CreateDescriptorSet(VkDescriptorPool l_dsPool, VkDescriptorSetLayout l_dsLayout,
		const char* l_dsSet)
	{
		using namespace ErrorCheck;

		VkDescriptorSet lv_ds{};

		VkDescriptorSetAllocateInfo lv_dsAllocateInfo{};
		lv_dsAllocateInfo.descriptorPool = l_dsPool;
		lv_dsAllocateInfo.descriptorSetCount = 1;
		lv_dsAllocateInfo.pSetLayouts = &l_dsLayout;
		lv_dsAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

		VULKAN_CHECK(vkAllocateDescriptorSets(m_renderDevice.m_device, &lv_dsAllocateInfo, &lv_ds));

		VkDebugUtilsObjectNameInfoEXT lv_objectNameInfo{};
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_ds);
		lv_objectNameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET;
		lv_objectNameInfo.pNext = nullptr;
		lv_objectNameInfo.pObjectName = l_dsSet;
		lv_objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;


		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));
		


		return lv_ds;

		
	}


	void VulkanResourceManager::UpdateDescriptorSet(const DescriptorSetResources& l_dsResources, 
		VkDescriptorSet l_ds)
	{
		using namespace ErrorCheck;

		uint32_t lv_bindingNumber{};
		std::vector<VkWriteDescriptorSet> lv_writeDSVector{};
		std::vector<VkDescriptorBufferInfo> lv_dsBufferInfoVector{ l_dsResources.m_buffers.size() };
		std::vector<VkDescriptorImageInfo> lv_textureImageInfoVector{ l_dsResources.m_textures.size() };
		std::vector<VkDescriptorImageInfo> lv_textureArrayImageInfoVector{};


		for (uint32_t i = 0; i < l_dsResources.m_buffers.size(); ++i) {

			lv_dsBufferInfoVector[i] = VkDescriptorBufferInfo{.buffer = l_dsResources.m_buffers[i].m_buffer.buffer
			,.offset = l_dsResources.m_buffers[i].m_offset, .range = l_dsResources.m_buffers[i].m_size};

			lv_writeDSVector.push_back(bufferWriteDescriptorSet(l_ds, &lv_dsBufferInfoVector[i], lv_bindingNumber++,
				l_dsResources.m_buffers[i].m_descriptorInfo.m_type));

		}

		for (uint32_t i = 0; i < l_dsResources.m_textures.size(); ++i) {

			lv_textureImageInfoVector[i] = VkDescriptorImageInfo{
				.sampler = l_dsResources.m_textures[i].m_texture.sampler,
				.imageView = l_dsResources.m_textures[i].m_texture.image.imageView,
				.imageLayout = l_dsResources.m_textures[i].m_texture.Layout};

			lv_writeDSVector.push_back(imageWriteDescriptorSet(l_ds, &lv_textureImageInfoVector[i], lv_bindingNumber++));

		}


		for (uint32_t i = 0; i < l_dsResources.m_textureArrays.size(); ++i) {
			for (uint32_t j = 0; j < l_dsResources.m_textureArrays[i].m_textures.size(); ++j) {
				lv_textureArrayImageInfoVector.push_back(VkDescriptorImageInfo{
					.sampler = l_dsResources.m_textureArrays[i].m_textures[j].sampler,
					.imageView = l_dsResources.m_textureArrays[i].m_textures[j].image.imageView,
					.imageLayout = l_dsResources.m_textureArrays[i].m_textures[j].Layout
					});
			}
		}

		uint32_t lv_textureOffset{};
		for (uint32_t i = 0; i < l_dsResources.m_textureArrays.size(); ++i) {
			lv_writeDSVector.push_back(VkWriteDescriptorSet{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = l_ds,
				.dstBinding = lv_bindingNumber++,
				.dstArrayElement = 0,
				.descriptorCount = (uint32_t)l_dsResources.m_textureArrays[i].m_textures.size(),
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = lv_textureArrayImageInfoVector.data() + lv_textureOffset,

				});

			lv_textureOffset += l_dsResources.m_textureArrays[i].m_textures.size();
		}

		vkUpdateDescriptorSets(m_renderDevice.m_device, lv_writeDSVector.size(),
			lv_writeDSVector.data(), 0, nullptr);

	}



	VulkanResourceManager::~VulkanResourceManager()
	{
		for (auto& l_buffer : m_buffers) {
			if (nullptr != l_buffer.ptr) {
				vkUnmapMemory(m_renderDevice.m_device, l_buffer.memory);
			}
			vkFreeMemory(m_renderDevice.m_device, l_buffer.memory, nullptr);
			vkDestroyBuffer(m_renderDevice.m_device, l_buffer.buffer, nullptr);
		}

		for (auto& l_texture : m_textures) {
			destroyVulkanTexture(m_renderDevice.m_device, l_texture);
		}

		for (auto& l_frameBuffer : m_frameBuffers) {
			vkDestroyFramebuffer(m_renderDevice.m_device, l_frameBuffer, nullptr);
		}

		for (auto& l_renderpass : m_renderPasses) {
			vkDestroyRenderPass(m_renderDevice.m_device, l_renderpass, nullptr);
		}

		for (auto& l_pipeline : m_Pipelines) {
			vkDestroyPipeline(m_renderDevice.m_device, l_pipeline, nullptr);
		}

		for (auto& l_pipelineLayout : m_pipelineLayouts) {
			vkDestroyPipelineLayout(m_renderDevice.m_device, l_pipelineLayout, nullptr);
		}

		for (auto& l_descriptorPool : m_descriptorPools) {
			vkDestroyDescriptorPool(m_renderDevice.m_device, l_descriptorPool, nullptr);
		}

		for (auto& l_descriptorSetLayout : m_descriptorSetLayouts) {
			vkDestroyDescriptorSetLayout(m_renderDevice.m_device, l_descriptorSetLayout, nullptr);
		}

	}

}