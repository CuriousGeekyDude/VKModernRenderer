#pragma once 



#include "UtilsVulkan.h"
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <unordered_map>
#include "ErrorCheck.hpp"


namespace VulkanEngine
{
	struct CameraApp;
}


namespace RenderCore
{
	class VulkanResourceManager final
	{
	
	public:

		struct RenderPass {

			RenderPass() = default;

			RenderPass(VulkanRenderDevice& l_device,
			bool l_useDepth = true,
			const RenderPassCreateInfo& l_ci =
			RenderPassCreateInfo()) : m_info(l_ci)
			{
				using namespace ErrorCheck;

				if (!createColorAndDepthRenderPass(l_device, l_useDepth, &m_renderpass, l_ci)) {
					PRINT_EXIT("\nFailed to create render pass.\n");
				}
			}

			RenderPass& operator=(const RenderPass& l_renderpass) = default;

			RenderPassCreateInfo m_info;
			VkRenderPass m_renderpass = VK_NULL_HANDLE;
		};

		struct PipelineInfo {

			PipelineInfo& operator=(const PipelineInfo&) = default;

			uint32_t m_width = 0;
			uint32_t m_height = 0;
			VkPrimitiveTopology m_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			bool m_useDepth = true;
			bool m_useBlending = true;
			bool m_dynamicScissorState = false;

			uint32_t m_totalNumColorAttach = 0;
		};


		struct DescriptorInfo
		{

			DescriptorInfo& operator=(const DescriptorInfo&) = default;

			VkDescriptorType m_type;
			VkShaderStageFlags m_shaderStageFlags;
		};



		struct BufferResourceShader
		{


			BufferResourceShader& operator=(const BufferResourceShader&) = default;


			DescriptorInfo m_descriptorInfo;
			VulkanBuffer   m_buffer;

			//We need the offset since sometimes we might split a buffer to more than 1 parts and use just one part
			//for this bufferResourceShader instance. For example, index and vertex buffers.
			uint32_t       m_offset;
			uint32_t       m_size;
		};


		struct TextureResourceShader
		{

			TextureResourceShader& operator=(const TextureResourceShader&) = default;

			DescriptorInfo m_descriptorInfo;
			VulkanTexture m_texture;
		};

		struct TextureArrayResourceShader
		{
			TextureArrayResourceShader& operator=(const TextureArrayResourceShader&) = default;


			DescriptorInfo m_descriptorInfo;
			std::vector<VulkanTexture> m_textures;
		};



		struct DescriptorSetResources
		{

			std::vector<BufferResourceShader> m_buffers;
			std::vector<TextureResourceShader> m_textures;
			std::vector<TextureArrayResourceShader> m_textureArrays;
		};


		enum class VulkanDataType
		{
			m_buffer,
			m_texture,
			m_framebuffer,
			m_renderpass,
			m_pipelineLayout,
			m_pipeline,
			m_descriptorPool,
			m_descriptorSetLayout,
			m_invalid
		};


		struct GpuResourceMetaData
		{
			VulkanDataType m_vkDataType;
			uint32_t m_resourceHandle;
		};


		

	public:

		explicit VulkanResourceManager(VulkanRenderDevice& l_renderDevice);

		VulkanBuffer& CreateSharedBuffer(VkDeviceSize l_size, VkBufferUsageFlags l_usage, 
			VkMemoryPropertyFlags l_memoryProperties, const char* l_nameBuffer);
		
		VulkanBuffer& CreateBuffer(VkDeviceSize l_size, VkBufferUsageFlags l_usage,
			VkMemoryPropertyFlags l_memoryProperties, const char* l_nameBuffer);



		void CopyDataToLocalBuffer(VkQueue l_queue, VkCommandBuffer l_cmdBuffer,
			const uint32_t l_bufferHandle, const void* l_dstBufferData);

		VkPipeline CreateComputePipeline(VkDevice m_device, const char* l_computeShaderFilePath
			,VkPipelineLayout pipelineLayout);

		uint32_t CreateBufferWithHandle(VkDeviceSize l_size, VkBufferUsageFlags l_usage,
			VkMemoryPropertyFlags l_memoryProperties, const char* l_nameBuffer);

		VulkanTexture& CreateTextureForOffscreenFrameBuffer(const std::string& l_nameTexture,
			VkFormat l_colorFormat = VK_FORMAT_B8G8R8A8_UNORM,
			VkFilter l_minFilter = VK_FILTER_LINEAR,
			VkFilter l_maxFilter = VK_FILTER_LINEAR,
			VkSamplerAddressMode l_addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);


		//Same as CreateTextureForOffscreenFrameBuffer() except this one returns the handle
		uint32_t CreateTexture(const char* l_nameTexture,
			VkFormat l_colorFormat = VK_FORMAT_B8G8R8A8_UNORM,
			VkFilter l_minFilter = VK_FILTER_LINEAR,
			VkFilter l_maxFilter = VK_FILTER_LINEAR,
			VkSamplerAddressMode l_addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);


		VulkanTexture& LoadTexture2D(const std::string& l_textureFileName);

		uint32_t LoadTexture2DWithHandle(const std::string& l_textureFileName);

		VulkanTexture& CreateDepthTextureForOffscreenFrameBuffer(const std::string& l_nameDepthTexture);

		VulkanTexture& CreateDepthTexture(const std::string& l_nameDepthTexture);


		uint32_t CreateDepthTextureWithHandle(const std::string& l_nameTexture);


		VkFramebuffer& CreateFrameBuffer(const RenderPass& l_renderpass, 
			const std::vector<VulkanTexture>& l_images,
			const char* l_nameFramebuffer);



		uint32_t CreateFrameBuffer(const RenderPass& l_renderpass,
			const std::vector<uint32_t>& l_textureHandles,
			const char* l_nameFramebuffer);




		//Create color only or color and depth render pass based on the number of textures 
		RenderPass CreateRenderPass(const std::vector<VulkanTexture>& l_textures,
			const char* l_nameRenderPass,
			const RenderPassCreateInfo l_ci = {
			  .clearColor_ = true, .clearDepth_ = true,
			  .flags_ = eRenderPassBit_First_ColorAttach},
			bool l_useDepth = true);



		RenderPass CreateDepthOnlyRenderPass(const std::vector<VulkanTexture>& l_textures,
			const char* l_nameRenderPass,
			const RenderPassCreateInfo l_ci = {
			  .clearColor_ = false, .clearDepth_ = true,
			  .flags_ = eRenderPassBit_First_ColorAttach });

		RenderPass CreateFullScreenRenderPass(bool l_useDepth, const RenderPassCreateInfo& l_ci,
			const char* l_nameRenderPass);
		std::vector<VkFramebuffer> CreateFullScreenFrameBuffers(
			VulkanResourceManager::RenderPass l_renderPass, VkImageView l_depthView);



		VkPipelineLayout& CreatePipelineLayoutWithPush(VkDescriptorSetLayout l_descriptorSetLayout,
			const char* l_namePipelineLayout,
			uint32_t l_vtxConstSize = 0, uint32_t l_fragConstSize = 0);


		VkPipelineLayout& CreatePipelineLayout(VkDescriptorSetLayout l_descriptorSetLayout,
			const char* l_namePipelineLayout);


		VkPipeline& CreateGraphicsPipeline(VkRenderPass l_renderPass, VkPipelineLayout l_pipelineLayout,
			const std::vector<const char*>& l_shaderFiles,
			const char* l_nameGraphicsPipeline,
			const PipelineInfo& l_pipelineParams = PipelineInfo{ .m_width = 0, .m_height = 0, 
			.m_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, .m_useDepth = true, 
			.m_useBlending = false, .m_dynamicScissorState = false });



		VkDescriptorSetLayout& CreateDescriptorSetLayout(const DescriptorSetResources& l_dsResources,
			const char* l_nameDsLayout);

		VkDescriptorPool& CreateDescriptorPool(const DescriptorSetResources& l_dsResources, 
			uint32_t l_totalNumDescriptorSets,
			const char* l_nameDsPool);

		VkDescriptorSet CreateDescriptorSet(VkDescriptorPool l_dsPool, VkDescriptorSetLayout l_dsLayout,
			const char* l_dsSet);


		void UpdateDescriptorSet(const DescriptorSetResources& l_dsResources, VkDescriptorSet l_ds);

		void AddGpuResource(const char* l_nameResource, uint32_t l_resourceHandle, VulkanDataType l_vkDataType);

		GpuResourceMetaData RetrieveGpuResourceMetaData(const std::string& l_nameResource);
		void* RetrieveGpuResource(const GpuResourceMetaData& l_resourceMetaData);

		VulkanBuffer& RetrieveGpuBuffer(const uint32_t l_handle);
		VulkanTexture& RetrieveGpuTexture(const uint32_t l_handle);
		VkFramebuffer RetrieveGpuFramebuffer(const uint32_t l_handle);
		VkRenderPass RetrieveGpuRenderpass(const uint32_t l_handle);
		VkPipelineLayout RetrieveGpuPipelineLayout(const uint32_t l_handle);
		VkPipeline	RetrieveGpuPipeline(const uint32_t l_handle);
		VkDescriptorSetLayout RetrieveGpuDescriptorSetLayout(const uint32_t l_handle);
		VkDescriptorPool RetrieveGpuDescriptorPool(const uint32_t l_handle);

		uint32_t AddVulkanBuffer(const VulkanBuffer& l_buffer);
		uint32_t AddVulkanTexture(const VulkanTexture& l_texture);
		uint32_t AddVulkanFramebuffer(VkFramebuffer l_framebuffer);
		uint32_t AddVulkanRenderpass(VkRenderPass l_renderpass);
		uint32_t AddVulkanPipelineLayout(VkPipelineLayout l_pipelineLayout);
		uint32_t AddVulkanPipeline(VkPipeline l_pipeline);
		uint32_t AddVulkanDescriptorSetLayout(VkDescriptorSetLayout l_dsSetLayout);
		uint32_t AddVulkanDescriptorPool(VkDescriptorPool l_dsPool);





		~VulkanResourceManager();

	private:

		std::vector<VulkanBuffer> m_buffers{};
		std::vector<VulkanTexture> m_textures{};
		std::vector<VkFramebuffer> m_frameBuffers{};
		std::vector<VkRenderPass> m_renderPasses{};
		std::vector<VkPipelineLayout> m_pipelineLayouts{};
		std::vector<VkPipeline> m_Pipelines{};
		std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts{};
		std::vector<VkDescriptorPool> m_descriptorPools{};

		std::unordered_map<std::string, GpuResourceMetaData> m_gpuResourcesHandles;

		VulkanRenderDevice& m_renderDevice;


	};
}