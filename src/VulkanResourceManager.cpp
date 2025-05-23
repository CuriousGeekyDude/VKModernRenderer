



#include "VulkanResourceManager.hpp"
#include "VulkanEngineCore.hpp"
#include <format>


namespace RenderCore
{

	VulkanResourceManager::VulkanResourceManager(VulkanRenderDevice& l_renderDevice)
		:m_renderDevice(l_renderDevice) {

		auto lv_totalNumSwapchhains = l_renderDevice.m_swapchainImages.size();

		m_buffers.reserve(64);
		m_textures.reserve(512);

		m_frameBuffers.reserve(64);
		m_renderPasses.reserve(64);

		m_descriptorPools.reserve(4);
		m_descriptorSetLayouts.reserve(32);

		m_pipelineLayouts.reserve(64);
		m_Pipelines.reserve(64);

		for (size_t i = 0; i < lv_totalNumSwapchhains; ++i) {

			VulkanTexture lv_swapchain{};
			lv_swapchain.depth = 0;
			lv_swapchain.format = VK_FORMAT_B8G8R8A8_SRGB;
			lv_swapchain.height = l_renderDevice.m_framebufferHeight;
			lv_swapchain.width = l_renderDevice.m_framebufferWidth;
			lv_swapchain.sampler = nullptr;
			lv_swapchain.image.image = l_renderDevice.m_swapchainImages[i];
			lv_swapchain.image.imageView0 = l_renderDevice.m_swapchainImageViews[i];

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



		CreateDepthCubeMapTexture("DepthMapPointLight", 1024 , 1024);


	}

	uint32_t VulkanResourceManager::CreateFrameBufferCubemapFace(const RenderPass& l_renderpass,
		uint32_t l_textureHandle,
		uint32_t l_cubemapLayer,
		const char* l_nameFramebuffer)
	{
		using namespace ErrorCheck;

		VkFramebuffer lv_frameBufferToCreate{};

		assert(l_textureHandle != std::numeric_limits<uint32_t>::max());
		assert(0 <= l_cubemapLayer && l_cubemapLayer <= 5);

		auto& lv_cubemapTexture = RetrieveGpuTexture(l_textureHandle);

		VkImageView lv_cubemapFaceView = VK_NULL_HANDLE;

		switch (l_cubemapLayer) {
		case 0:
			lv_cubemapFaceView = lv_cubemapTexture.image.imageView0;
			break;
		case 1:
			lv_cubemapFaceView = lv_cubemapTexture.image.imageView1;
			break;
		case 2:
			lv_cubemapFaceView = lv_cubemapTexture.image.imageView2;
			break;
		case 3:
			lv_cubemapFaceView = lv_cubemapTexture.image.imageView3;
			break;
		case 4:
			lv_cubemapFaceView = lv_cubemapTexture.image.imageView4;
			break;
		case 5:
			lv_cubemapFaceView = lv_cubemapTexture.image.imageView5;
			break;

		}

		assert(VK_NULL_HANDLE != lv_cubemapFaceView);

		VkFramebufferCreateInfo lv_frameBufferCreateInfo{};
		lv_frameBufferCreateInfo.attachmentCount = 1;
		lv_frameBufferCreateInfo.height = lv_cubemapTexture.height;
		lv_frameBufferCreateInfo.width = lv_cubemapTexture.width;
		lv_frameBufferCreateInfo.pAttachments = &lv_cubemapFaceView;
		lv_frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		lv_frameBufferCreateInfo.renderPass = l_renderpass.m_renderpass;
		lv_frameBufferCreateInfo.layers = 1U;

		VULKAN_CHECK(vkCreateFramebuffer(m_renderDevice.m_device, &lv_frameBufferCreateInfo, nullptr, &lv_frameBufferToCreate));

		m_frameBuffers.push_back(lv_frameBufferToCreate);


		VkDebugUtilsObjectNameInfoEXT lv_objectNameInfo{};
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_frameBufferToCreate);
		lv_objectNameInfo.objectType = VK_OBJECT_TYPE_FRAMEBUFFER;
		lv_objectNameInfo.pNext = nullptr;
		lv_objectNameInfo.pObjectName = l_nameFramebuffer;
		lv_objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;


		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));


		return (uint32_t)m_frameBuffers.size() - 1;
	}




	VulkanTexture& VulkanResourceManager::CreateDepthCubeMapTexture(const std::string& l_textureName, uint32_t l_height
		, uint32_t l_width)
	{

		using namespace ErrorCheck;

		VulkanTexture lv_depthTextureToCreate{};
		lv_depthTextureToCreate.height = l_height;
		lv_depthTextureToCreate.width = l_width;
		lv_depthTextureToCreate.format = findDepthFormat(m_renderDevice.m_physicalDevice);
		lv_depthTextureToCreate.depth = 1U;


		if (false == createImage(m_renderDevice.m_device, m_renderDevice.m_physicalDevice, lv_depthTextureToCreate.width,
			lv_depthTextureToCreate.height, lv_depthTextureToCreate.format, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			lv_depthTextureToCreate.image.image, lv_depthTextureToCreate.image.imageMemory
			, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)) {
			PRINT_EXIT("\nFailed to create depth texture.\n");
		}


		VkDebugUtilsObjectNameInfoEXT lv_objectNameInfo{};
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_depthTextureToCreate.image.image);
		lv_objectNameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
		lv_objectNameInfo.pNext = nullptr;
		lv_objectNameInfo.pObjectName = l_textureName.c_str();
		lv_objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;


		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));


		if (false == createImageViewCubeMap(m_renderDevice.m_device, lv_depthTextureToCreate.image.image,
			lv_depthTextureToCreate.format, VK_IMAGE_ASPECT_DEPTH_BIT, &lv_depthTextureToCreate.image.imageView0
									, VK_IMAGE_VIEW_TYPE_2D)) {
			PRINT_EXIT("\nFaled to create image view of depth texture\n");
		}
		if (false == createImageViewCubeMap(m_renderDevice.m_device, lv_depthTextureToCreate.image.image,
			lv_depthTextureToCreate.format, VK_IMAGE_ASPECT_DEPTH_BIT, &lv_depthTextureToCreate.image.imageView1
			, VK_IMAGE_VIEW_TYPE_2D, 1)) {
			PRINT_EXIT("\nFaled to create image view of depth texture\n");
		}
		if (false == createImageViewCubeMap(m_renderDevice.m_device, lv_depthTextureToCreate.image.image,
			lv_depthTextureToCreate.format, VK_IMAGE_ASPECT_DEPTH_BIT, &lv_depthTextureToCreate.image.imageView2
			, VK_IMAGE_VIEW_TYPE_2D, 2)) {
			PRINT_EXIT("\nFaled to create image view of depth texture\n");
		}
		if (false == createImageViewCubeMap(m_renderDevice.m_device, lv_depthTextureToCreate.image.image,
			lv_depthTextureToCreate.format, VK_IMAGE_ASPECT_DEPTH_BIT, &lv_depthTextureToCreate.image.imageView3
			, VK_IMAGE_VIEW_TYPE_2D, 3)) {
			PRINT_EXIT("\nFaled to create image view of depth texture\n");
		}
		if (false == createImageViewCubeMap(m_renderDevice.m_device, lv_depthTextureToCreate.image.image,
			lv_depthTextureToCreate.format, VK_IMAGE_ASPECT_DEPTH_BIT, &lv_depthTextureToCreate.image.imageView4
			, VK_IMAGE_VIEW_TYPE_2D, 4)) {
			PRINT_EXIT("\nFaled to create image view of depth texture\n");
		}
		if (false == createImageViewCubeMap(m_renderDevice.m_device, lv_depthTextureToCreate.image.image,
			lv_depthTextureToCreate.format, VK_IMAGE_ASPECT_DEPTH_BIT, &lv_depthTextureToCreate.image.imageView5
			, VK_IMAGE_VIEW_TYPE_2D, 5)) {
			PRINT_EXIT("\nFaled to create image view of depth texture\n");
		}

		if (false == createImageView(m_renderDevice.m_device, lv_depthTextureToCreate.image.image,
			lv_depthTextureToCreate.format, VK_IMAGE_ASPECT_DEPTH_BIT, &lv_depthTextureToCreate.image.cubemapImageView
			, VK_IMAGE_VIEW_TYPE_CUBE, 6)) {
			PRINT_EXIT("\nFaled to create image view of depth texture\n");
		}

		std::string lv_imageViewName{ l_textureName + "-view0 " };
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_depthTextureToCreate.image.imageView0);
		lv_objectNameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
		lv_objectNameInfo.pNext = nullptr;
		lv_objectNameInfo.pObjectName = lv_imageViewName.c_str();
		lv_objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;


		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));

		lv_imageViewName = l_textureName + "-view1 ";
		lv_objectNameInfo.pObjectName = lv_imageViewName.c_str();
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_depthTextureToCreate.image.imageView1);
		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));

		lv_imageViewName = l_textureName + "-view2 ";
		lv_objectNameInfo.pObjectName = lv_imageViewName.c_str();
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_depthTextureToCreate.image.imageView2);
		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));

		lv_imageViewName = l_textureName + "-view3 ";
		lv_objectNameInfo.pObjectName = lv_imageViewName.c_str();
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_depthTextureToCreate.image.imageView3);
		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));

		lv_imageViewName = l_textureName + "-view4 ";
		lv_objectNameInfo.pObjectName = lv_imageViewName.c_str();
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_depthTextureToCreate.image.imageView4);
		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));

		lv_imageViewName = l_textureName + "-view5 ";
		lv_objectNameInfo.pObjectName = lv_imageViewName.c_str();
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_depthTextureToCreate.image.imageView5);
		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));

		lv_imageViewName = l_textureName + "-viewCubemap ";
		lv_objectNameInfo.pObjectName = lv_imageViewName.c_str();
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_depthTextureToCreate.image.cubemapImageView);
		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));


		if (false == createTextureSampler(m_renderDevice.m_device, &lv_depthTextureToCreate.sampler ,1.f
										, 1.f, VK_FILTER_LINEAR, VK_FILTER_LINEAR
										, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER)) {
			PRINT_EXIT("\nFailed to create image sampler for depth texture.\n");
		}


		std::string lv_samplerName{ l_textureName + "-sampler " };
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_depthTextureToCreate.sampler);
		lv_objectNameInfo.objectType = VK_OBJECT_TYPE_SAMPLER;
		lv_objectNameInfo.pNext = nullptr;
		lv_objectNameInfo.pObjectName = lv_samplerName.c_str();
		lv_objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;


		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo));


		transitionImageLayout(m_renderDevice, lv_depthTextureToCreate.image.image, lv_depthTextureToCreate.format,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6);

		lv_depthTextureToCreate.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		lv_depthTextureToCreate.l_cubemapFace0Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		lv_depthTextureToCreate.l_cubemapFace1Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		lv_depthTextureToCreate.l_cubemapFace2Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		lv_depthTextureToCreate.l_cubemapFace3Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		lv_depthTextureToCreate.l_cubemapFace4Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		lv_depthTextureToCreate.l_cubemapFace5Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;


		m_textures.push_back(lv_depthTextureToCreate);

		GpuResourceMetaData lv_textureMetaData{};
		lv_textureMetaData.m_resourceHandle = (uint32_t)m_textures.size() - 1;
		lv_textureMetaData.m_vkDataType = VulkanDataType::m_texture;

		m_gpuResourcesHandles.emplace(std::string{ l_textureName }, lv_textureMetaData);

		return m_textures[m_textures.size() - 1];

	}


	void VulkanResourceManager::CopyDataToLocalBuffer(VkQueue l_queue, VkCommandBuffer l_cmdBuffer,
		const uint32_t l_bufferHandle, const void* l_bufferData)
	{
		using namespace ErrorCheck;


		auto& lv_buffer = RetrieveGpuBuffer(l_bufferHandle);

		auto& lv_stagingBuffer = CreateBuffer(lv_buffer.size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			, "TemporaryStagingBufferForDataCopy ");


		memcpy(lv_stagingBuffer.ptr, l_bufferData, lv_buffer.size);


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

		std::string lv_temp{ l_nameBuffer };
		GpuResourceMetaData lv_metaData{};
		lv_metaData.m_resourceHandle = m_buffers.size() - 1;
		lv_metaData.m_vkDataType = VulkanDataType::m_buffer;

		m_gpuResourcesHandles.emplace(std::move(lv_temp), lv_metaData);

		return m_buffers[m_buffers.size()-1];
	}




	VulkanTexture& 
		VulkanResourceManager::CreateTextureForOffscreenFrameBuffer(float l_maxAnistropy ,const std::string& l_nameTexture,
		VkFormat l_colorFormat, 
		uint32_t l_width, uint32_t l_height,
		uint32_t l_mipLevels,
		VkFilter l_minFilter,
		VkFilter l_maxFilter,
		VkSamplerAddressMode l_addressMode)
	{
		using namespace ErrorCheck;

		VulkanTexture lv_textureToCreate{};
		lv_textureToCreate.format = l_colorFormat;
		lv_textureToCreate.height = l_height;
		lv_textureToCreate.width = l_width;
		lv_textureToCreate.depth = 1U;

		if (false == createOffscreenImage(m_renderDevice, lv_textureToCreate.image.image,
			lv_textureToCreate.image.imageMemory, lv_textureToCreate.width, lv_textureToCreate.height,
			l_colorFormat, 1, 0, l_mipLevels)) {
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
			l_colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, &lv_textureToCreate.image.imageView0)) {
			PRINT_EXIT(".\nFailed to create image view of an offscreen attachment.\n");
		}


		std::string lv_imageViewName{ l_nameTexture + "-view " };
		VkDebugUtilsObjectNameInfoEXT lv_objectNameInfo1{};
		lv_objectNameInfo1.objectHandle = reinterpret_cast<uint64_t>(lv_textureToCreate.image.imageView0);
		lv_objectNameInfo1.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
		lv_objectNameInfo1.pNext = nullptr;
		lv_objectNameInfo1.pObjectName = lv_imageViewName.c_str();
		lv_objectNameInfo1.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;


		VULKAN_CHECK(vkSetDebugUtilsObjectNameEXT(m_renderDevice.m_device, &lv_objectNameInfo1));

		if (false == createTextureSampler(m_renderDevice.m_device, &lv_textureToCreate.sampler, (float)l_mipLevels,l_maxAnistropy,l_minFilter,
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
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,1, 1,0);

		if (1 < l_mipLevels) {
			transitionImageLayout(m_renderDevice, lv_textureToCreate.image.image, l_colorFormat, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, l_mipLevels-1, 1);
		}

		lv_textureToCreate.Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		m_textures.push_back(lv_textureToCreate);




		return m_textures[m_textures.size() - 1];

	}


	VulkanTexture& VulkanResourceManager::LoadTexture2D(const std::string& l_textureFileName)
	{
		
		using namespace ErrorCheck;

		VulkanTexture lv_textureToCreate{};
		uint32_t lv_mipLevel{std::numeric_limits<uint32_t>::max()};



		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(m_renderDevice.m_physicalDevice, VK_FORMAT_R8G8B8A8_UNORM,
			&formatProperties);

		if ((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) == 0) {
			PRINT_EXIT("\nLinear filtering is not supported for VK_FORMAT_R8G8B8A8_UNORM format. Exitting...\n");
		}

		printf("\nAttempting to load : %s\n", l_textureFileName.c_str());

		if (false == createTextureImage(m_renderDevice, l_textureFileName.c_str(),
			lv_textureToCreate.image.image, lv_textureToCreate.image.imageMemory,
			&lv_textureToCreate.width, &lv_textureToCreate.height, &lv_mipLevel)) {

			PRINT_EXIT("\nFailed to create texture image from texture file name.\n");

		}

		if (std::numeric_limits<uint32_t>::max() == lv_mipLevel) {
			PRINT_EXIT("\nFailed to retrieve the mipmap level from loading texture.\n");

		}

		if (false == createImageView(m_renderDevice.m_device, lv_textureToCreate.image.image,
			VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &lv_textureToCreate.image.imageView0, VK_IMAGE_VIEW_TYPE_2D, 1, lv_mipLevel)) {
			PRINT_EXIT("\nFailed to create image view for the loaded 2D texture file.\n");
		}

		if (false == createTextureSampler(m_renderDevice.m_device, &lv_textureToCreate.sampler, (float)lv_mipLevel)) {
			PRINT_EXIT("\nFailed to create sampler for loaded 2D texture.\n");
		}

		transitionImageLayout(m_renderDevice, lv_textureToCreate.image.image, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,1, 1, 0);

		
		transitionImageLayout(m_renderDevice, lv_textureToCreate.image.image, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, lv_mipLevel - 1, 1);


		lv_textureToCreate.format = VK_FORMAT_R8G8B8A8_UNORM;
		lv_textureToCreate.depth = 1U;

		int32_t lv_srcMipmapWidth = (int32_t)lv_textureToCreate.width;
		int32_t lv_srcMipmapHeight = (int32_t)lv_textureToCreate.height;

		//Downsample from mip chain n-1 to n
		for (uint32_t i = 1; i < lv_mipLevel; ++i) {

			VkImageBlit lv_imageBlit{};

			lv_imageBlit.srcOffsets[0] = {0,0,0};
			lv_imageBlit.srcOffsets[1] = {lv_srcMipmapWidth, lv_srcMipmapHeight, 1};
			lv_imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			lv_imageBlit.srcSubresource.baseArrayLayer = 0;
			lv_imageBlit.srcSubresource.layerCount = 1;
			lv_imageBlit.srcSubresource.mipLevel = i - 1;

			lv_srcMipmapWidth = lv_srcMipmapWidth == 1 ? 1 : lv_srcMipmapWidth / 2;
			lv_srcMipmapHeight = lv_srcMipmapHeight == 1 ? 1 : lv_srcMipmapHeight / 2;
			lv_imageBlit.dstOffsets[0] = { 0,0,0 };
			lv_imageBlit.dstOffsets[1] = {lv_srcMipmapWidth, lv_srcMipmapHeight, 1};
			lv_imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			lv_imageBlit.dstSubresource.baseArrayLayer = 0;
			lv_imageBlit.dstSubresource.layerCount = 1;
			lv_imageBlit.dstSubresource.mipLevel = i;

			VkCommandBuffer commandBuffer = beginSingleTimeCommands(m_renderDevice);
			vkCmdBlitImage(commandBuffer, lv_textureToCreate.image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
						  , lv_textureToCreate.image.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1, &lv_imageBlit, VK_FILTER_LINEAR);
			endSingleTimeCommands(m_renderDevice, commandBuffer);


			transitionImageLayout(m_renderDevice, lv_textureToCreate.image.image
				, lv_textureToCreate.format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
				, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1, 1, i);

		}

		transitionImageLayout(m_renderDevice, lv_textureToCreate.image.image
			, lv_textureToCreate.format, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, lv_mipLevel, 0);


		
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
			lv_depthTextureToCreate.format, VK_IMAGE_ASPECT_DEPTH_BIT, &lv_depthTextureToCreate.image.imageView0)) {
			PRINT_EXIT("\nFaled to create image view of depth texture\n");
		}


		std::string lv_imageViewName{ l_nameDepthTexture + "-view " };
		VkDebugUtilsObjectNameInfoEXT lv_objectNameInfo1{};
		lv_objectNameInfo1.objectHandle = reinterpret_cast<uint64_t>(lv_depthTextureToCreate.image.imageView0);
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



	uint32_t VulkanResourceManager::CreateTexture(float l_maxAnistropy ,const char* l_nameTexture,
		VkFormat l_colorFormat,
		uint32_t l_width , uint32_t l_height,
		uint32_t l_mipLevels,
		VkFilter l_minFilter,
		VkFilter l_maxFilter,
		VkSamplerAddressMode l_addressMode)
	{
		CreateTextureForOffscreenFrameBuffer(l_maxAnistropy,l_nameTexture, l_colorFormat, l_width, l_height, l_mipLevels,l_minFilter, l_maxFilter, l_addressMode);

		return (uint32_t)m_textures.size()-1;
	}



	VulkanBuffer& VulkanResourceManager::RetrieveGpuBuffer
	(const std::string& l_bufferBaseName, const uint32_t l_index)
	{
		auto lv_formatedArg = std::make_format_args(l_index);
		std::string lv_formattedString{ l_bufferBaseName + " {}" };

		auto lv_bufferMeta = RetrieveGpuResourceMetaData(std::vformat(lv_formattedString, lv_formatedArg).c_str());

		if (lv_bufferMeta.m_vkDataType == VulkanDataType::m_invalid) {
			printf("Gpu buffer was not found. Exitting....");
			exit(-1);
		}

		auto& lv_bufferGpu = RetrieveGpuBuffer(lv_bufferMeta.m_resourceHandle);

		return lv_bufferGpu;
	}

	VulkanTexture& VulkanResourceManager::RetrieveGpuTexture
	(const std::string& l_textureBaseName, const uint32_t l_index)
	{
		auto lv_formatedArg = std::make_format_args(l_index);
		std::string lv_formattedString{ l_textureBaseName + " {}" };

		auto lv_textureMeta = RetrieveGpuResourceMetaData
		(std::vformat(lv_formattedString, lv_formatedArg).c_str());

		if (lv_textureMeta.m_vkDataType == VulkanDataType::m_invalid) {
			printf("Gpu texture was not found. Exitting....");
			exit(-1);
		}

		auto& lv_textureGpu = RetrieveGpuTexture(lv_textureMeta.m_resourceHandle);

		return lv_textureGpu;
	}

	VkFramebuffer VulkanResourceManager::RetrieveGpuFramebuffer
	(const std::string& l_framebufferBaseName, const uint32_t l_index)
	{
		auto lv_formatedArg = std::make_format_args(l_index);
		std::string lv_formattedString{ l_framebufferBaseName + " {}" };

		auto lv_framebufferMeta = RetrieveGpuResourceMetaData
		(std::vformat(lv_formattedString, lv_formatedArg).c_str());

		if (lv_framebufferMeta.m_vkDataType == VulkanDataType::m_invalid) {
			printf("Gpu framebuffer was not found. Exitting....");
			exit(-1);
		}

		auto lv_framebufferGpu = RetrieveGpuFramebuffer(lv_framebufferMeta.m_resourceHandle);

		return lv_framebufferGpu;
	}

	VkRenderPass VulkanResourceManager::RetrieveGpuRenderpass
	(const std::string& l_renderpassBaseName, const uint32_t l_index)
	{
		auto lv_formatedArg = std::make_format_args(l_index);
		std::string lv_formattedString{ l_renderpassBaseName + " {}" };

		auto lv_renderpassMeta = RetrieveGpuResourceMetaData
		(std::vformat(lv_formattedString, lv_formatedArg).c_str());

		if (lv_renderpassMeta.m_vkDataType == VulkanDataType::m_invalid) {
			printf("Gpu renderpass was not found. Exitting....");
			exit(-1);
		}

		auto lv_renderpassGpu = RetrieveGpuRenderpass(lv_renderpassMeta.m_resourceHandle);

		return lv_renderpassGpu;
	}

	VkPipelineLayout VulkanResourceManager::RetrieveGpuPipelineLayout
	(const std::string& l_pipelineLayoutBaseName, const uint32_t l_index)
	{
		auto lv_formatedArg = std::make_format_args(l_index);
		std::string lv_formattedString{ l_pipelineLayoutBaseName + " {}" };

		auto lv_pipelineLayoutMeta = RetrieveGpuResourceMetaData
		(std::vformat(lv_formattedString, lv_formatedArg).c_str());

		if (lv_pipelineLayoutMeta.m_vkDataType == VulkanDataType::m_invalid) {
			printf("Gpu pipeline layout was not found. Exitting....");
			exit(-1);
		}

		auto lv_pipelineLayoutGpu = RetrieveGpuPipelineLayout(lv_pipelineLayoutMeta.m_resourceHandle);

		return lv_pipelineLayoutGpu;
	}

	VkPipeline VulkanResourceManager::RetrieveGpuPipeline
	(const std::string& l_pipelineBaseName, const uint32_t l_index)
	{
		auto lv_formatedArg = std::make_format_args(l_index);
		std::string lv_formattedString{ l_pipelineBaseName + " {}" };

		auto lv_pipelineMeta = RetrieveGpuResourceMetaData
		(std::vformat(lv_formattedString, lv_formatedArg).c_str());

		if (lv_pipelineMeta.m_vkDataType == VulkanDataType::m_invalid) {
			printf("Gpu pipeline was not found. Exitting....");
			exit(-1);
		}

		auto lv_pipelineGpu = RetrieveGpuPipeline(lv_pipelineMeta.m_resourceHandle);

		return lv_pipelineGpu;
	}

	VkDescriptorSetLayout VulkanResourceManager::RetrieveGpuDescriptorSetLayout
	(const std::string& l_dsSetLayoutBaseName, const uint32_t l_index)
	{
		auto lv_formatedArg = std::make_format_args(l_index);
		std::string lv_formattedString{ l_dsSetLayoutBaseName + " {}" };

		auto lv_descriptorSetLayoutMeta = RetrieveGpuResourceMetaData
		(std::vformat(lv_formattedString, lv_formatedArg).c_str());

		if (lv_descriptorSetLayoutMeta.m_vkDataType == VulkanDataType::m_invalid) {
			printf("Gpu descriptor set layout was not found. Exitting....");
			exit(-1);
		}

		auto lv_descriptorSetLayoutGpu = RetrieveGpuDescriptorSetLayout
		(lv_descriptorSetLayoutMeta.m_resourceHandle);

		return lv_descriptorSetLayoutGpu;
	}

	VkDescriptorPool VulkanResourceManager::RetrieveGpuDescriptorPool
	(const std::string& l_descriptorPoolBaseName, const uint32_t l_index)
	{
		auto lv_formatedArg = std::make_format_args(l_index);
		std::string lv_formattedString{ l_descriptorPoolBaseName + " {}" };

		auto lv_descriptorPoolMeta = RetrieveGpuResourceMetaData
		(std::vformat(lv_formattedString, lv_formatedArg).c_str());

		if (lv_descriptorPoolMeta.m_vkDataType == VulkanDataType::m_invalid) {
			printf("Gpu descriptor pool was not found. Exitting....");
			exit(-1);
		}

		auto lv_descriptorPoolGpu = RetrieveGpuDescriptorPool
		(lv_descriptorPoolMeta.m_resourceHandle);

		return lv_descriptorPoolGpu;
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
		lv_depthTextureToCreate.height = 1024;
		lv_depthTextureToCreate.width = 1024;
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
			lv_depthTextureToCreate.format, VK_IMAGE_ASPECT_DEPTH_BIT, &lv_depthTextureToCreate.image.imageView0)) {
			PRINT_EXIT("\nFaled to create image view of depth texture\n");
		}


		std::string lv_imageViewName{ l_nameDepthTexture + "-view "};
		lv_objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(lv_depthTextureToCreate.image.imageView0);
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
			lv_attachments.push_back(l_image.image.imageView0);
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
			l_pipelineParams.m_height,0,
			l_pipelineParams.m_vertexInputBindingDescription,
			l_pipelineParams.m_vertexInputAttribDescription,
			l_pipelineParams.m_enableWireframe)) {
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
				.imageView = l_dsResources.m_textures[i].m_texture.image.imageView0,
				.imageLayout = l_dsResources.m_textures[i].m_texture.Layout};

			lv_writeDSVector.push_back(imageWriteDescriptorSet(l_ds, &lv_textureImageInfoVector[i], lv_bindingNumber++));

		}


		for (uint32_t i = 0; i < l_dsResources.m_textureArrays.size(); ++i) {
			for (uint32_t j = 0; j < l_dsResources.m_textureArrays[i].m_textures.size(); ++j) {
				lv_textureArrayImageInfoVector.push_back(VkDescriptorImageInfo{
					.sampler = l_dsResources.m_textureArrays[i].m_textures[j].sampler,
					.imageView = l_dsResources.m_textureArrays[i].m_textures[j].image.imageView0,
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
		auto lv_totalNumSwapchains = m_renderDevice.m_swapchainImages.size();
		for (auto& l_buffer : m_buffers) {
			
			if (nullptr != l_buffer.ptr) {
				vkUnmapMemory(m_renderDevice.m_device, l_buffer.memory);
			}
			vkFreeMemory(m_renderDevice.m_device, l_buffer.memory, nullptr);
			vkDestroyBuffer(m_renderDevice.m_device, l_buffer.buffer, nullptr);
		}

		for (auto& l_texture : m_textures) {


			if (0 == lv_totalNumSwapchains) {
				destroyVulkanTexture(m_renderDevice.m_device, l_texture);
				continue;
			}
			--lv_totalNumSwapchains;
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