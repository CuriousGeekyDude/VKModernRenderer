




#include "DeferredComputeRenderer.hpp"
#include "SpirvPipelineGenerator.hpp"
#include "CameraStructure.hpp"
#include <algorithm>
#include <array>
#include <format>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


namespace RenderCore
{

	DeferredComputeRenderer::DeferredComputeRenderer(VulkanEngine::VulkanRenderContext& l_vkContextCreator,
		const char* l_computeShaderFilePath,
		const std::string& l_spirvFile)
		:Renderbase(l_vkContextCreator)
	{

		using namespace VulkanEngine;

		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();
		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();
		uint32_t lv_screenWidth = (uint32_t)m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferWidth;
		uint32_t lv_screenHeight = (uint32_t)m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferHeight;

		m_tileBuffersHandles.resize(lv_totalNumSwapchains);
		m_uniformBuffersLightDataHandles.resize(lv_totalNumSwapchains);
		m_uniformBuffersCameraHandles.resize(lv_totalNumSwapchains);
		m_samplingTexturesHandles.resize(3*lv_totalNumSwapchains);
		m_outputImageTexturesHandles.resize(lv_totalNumSwapchains);

		m_uniformBuffersCPU.resize(lv_totalNumSwapchains);
		m_tileBuffersCPU.resize(lv_totalNumSwapchains);

		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
			m_tileBuffersCPU[i].resize(lv_screenWidth * lv_screenHeight / 8);

		}


		for (size_t i = 0, j = 0; i < lv_totalNumSwapchains; ++i, j+=3) {
			m_tileBuffersHandles[i] = lv_vkResManager.CreateBufferWithHandle( (lv_screenWidth*lv_screenHeight/ 8) * sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, std::format(" DeferredRendererTileBuffer {} ", i).c_str());
			m_uniformBuffersLightDataHandles[i] = lv_vkResManager.CreateBufferWithHandle(sizeof(UniformBuffer),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				std::format(" DeferredRendererUniformBufferLightData {} ",i).c_str());
			m_uniformBuffersCameraHandles[i] = lv_vkResManager.CreateBufferWithHandle(sizeof(DeferredUniformBufferCamera),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				std::format(" DeferredRendererUniformBufferCamera {} ", i).c_str());

			auto lv_posAttachMeta = lv_vkResManager.RetrieveGpuResourceMetaData(std::format("GBufferPosition {}", i).c_str());
			auto lv_normalAttachNameMeta = lv_vkResManager.RetrieveGpuResourceMetaData(std::format("GBufferNormal {}", i).c_str());
			auto lv_albedoSpecAttachMeta = lv_vkResManager.RetrieveGpuResourceMetaData(std::format("GBufferAlbedoSpec {}", i).c_str());

			m_samplingTexturesHandles[j] = lv_posAttachMeta.m_resourceHandle;
			m_samplingTexturesHandles[j+1] = lv_normalAttachNameMeta.m_resourceHandle;
			m_samplingTexturesHandles[j+2] = lv_albedoSpecAttachMeta.m_resourceHandle;

			m_outputImageTexturesHandles[i] = lv_vkResManager.CreateTexture(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_maxAnisotropy, std::format(" DeferredRendererOutputImage {} ", i).c_str());

			lv_vkResManager.AddGpuResource(std::format(" DeferredRendererOutputImage {} ", i).c_str(), 
				m_outputImageTexturesHandles[i], VulkanResourceManager::VulkanDataType::m_texture);

			auto& lv_outputImageTexture = lv_vkResManager.RetrieveGpuTexture(m_outputImageTexturesHandles[i]);

			transitionImageLayout(m_vulkanRenderContext.GetContextCreator().m_vkDev, lv_outputImageTexture.image.image,
				lv_outputImageTexture.format, lv_outputImageTexture.Layout, VK_IMAGE_LAYOUT_GENERAL);
			lv_outputImageTexture.Layout = VK_IMAGE_LAYOUT_GENERAL;
			
		}

		GeneratePipelineFromSpirvBinaries(l_spirvFile);

		auto* lv_node = lv_frameGraph.RetrieveNode("DeferredCompute");

		if (nullptr == lv_node) {
			printf("Indirect renderer was not found among the nodes of the frame graph. Exitting....\n");
			exit(-1);
		}
		lv_node->m_renderer = this;

	
		UpdateDescriptorSets();

		m_pipelineLayout = lv_vkResManager.CreatePipelineLayout(m_descriptorSetLayout, " DeferredRendererPipelineLayout ");

		m_computePipeline = lv_vkResManager.CreateComputePipeline(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device,
			l_computeShaderFilePath, m_pipelineLayout);


		

	}

	void DeferredComputeRenderer::FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
		uint32_t l_currentSwapchainIndex)
	{
		vkCmdBindPipeline(l_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);
		vkCmdBindDescriptorSets(l_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1,
			&m_descriptorSets[l_currentSwapchainIndex], 0, nullptr);

		uint32_t lv_width = (uint32_t)m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferWidth;
		uint32_t lv_height = (uint32_t)m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferHeight;




		vkCmdDispatch(l_cmdBuffer, (uint32_t)lv_width/(uint32_t)8 ,(uint32_t)lv_height/(uint32_t)8, 1);
		
	}

	/*void DeferredComputeRenderer::CreateFramebuffers()
	{
		auto& lv_vkResourceManager = m_vulkanRenderContext.GetResourceManager();
		m_framebufferHandles.resize(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size());

		for (uint32_t i = 0; i < (uint32_t)m_framebufferHandles.size(); ++i) {

			std::string lv_formattedString{ " swapchainDeferred {} " };
			auto lv_formattedArgs = std::make_format_args(i);

			m_framebufferHandles[i] = lv_vkResourceManager.CreateFrameBuffer(m_renderPass, { i }, std::vformat(lv_formattedString, lv_formattedArgs).c_str());
		}
	}*/
	void DeferredComputeRenderer::UpdateDescriptorSets()
	{
		constexpr uint32_t lv_totalNumBindings = 7;
		constexpr uint32_t lv_totalNumBuffers = 3;
		constexpr uint32_t lv_totalNumTextures = 4;
		auto lv_totalNumSwapchains = (uint32_t)m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();
		auto& lv_vkResourceManager = m_vulkanRenderContext.GetResourceManager();

		std::vector<VkDescriptorBufferInfo> lv_bufferInfos{};
		lv_bufferInfos.resize(lv_totalNumBuffers*lv_totalNumSwapchains);

		std::vector<VkDescriptorImageInfo> lv_imageInfos{};
		lv_imageInfos.resize(lv_totalNumTextures * lv_totalNumSwapchains);

		std::vector<VkWriteDescriptorSet> lv_writes{};
		lv_writes.resize(lv_totalNumBindings * lv_totalNumSwapchains);

		for (uint32_t i = 0, j = 0; i < lv_bufferInfos.size() && j < lv_totalNumSwapchains; i+=lv_totalNumBuffers, ++j) {

			auto& lv_tileBufferGPU = lv_vkResourceManager.RetrieveGpuBuffer(m_tileBuffersHandles[j]);
			auto& lv_uniformBufferLightGPU = lv_vkResourceManager.RetrieveGpuBuffer(m_uniformBuffersLightDataHandles[j]);
			auto& lv_uniformBufferCameraGPU = lv_vkResourceManager.RetrieveGpuBuffer(m_uniformBuffersCameraHandles[j]);

			lv_bufferInfos[i].buffer = lv_tileBufferGPU.buffer;
			lv_bufferInfos[i].offset = 0;
			lv_bufferInfos[i].range = lv_tileBufferGPU.size;

			lv_bufferInfos[i+1].buffer = lv_uniformBufferCameraGPU.buffer;  
			lv_bufferInfos[i+1].offset = 0;
			lv_bufferInfos[i+1].range = lv_uniformBufferCameraGPU.size;

			lv_bufferInfos[i+2].buffer = lv_uniformBufferLightGPU.buffer;
			lv_bufferInfos[i+2].offset = 0;
			lv_bufferInfos[i+2].range = lv_uniformBufferLightGPU.size;
		}

		for (uint32_t i = 0, j = 0, d = 0; i < lv_imageInfos.size() && j < m_samplingTexturesHandles.size();
			i += lv_totalNumTextures, j+=3, ++d) {

			auto& lv_gBufferPositonsGPU = lv_vkResourceManager.RetrieveGpuTexture(m_samplingTexturesHandles[j]);
			auto& lv_gbufferNormalGPU = lv_vkResourceManager.RetrieveGpuTexture(m_samplingTexturesHandles[j + 1]);;
			auto& lv_gbufferAlbedoSpecGPU = lv_vkResourceManager.RetrieveGpuTexture(m_samplingTexturesHandles[j + 2]);

			auto& lv_outputImage = lv_vkResourceManager.RetrieveGpuTexture(m_outputImageTexturesHandles[d]);


			lv_imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfos[i].imageView = lv_gBufferPositonsGPU.image.imageView;
			lv_imageInfos[i].sampler = lv_gBufferPositonsGPU.sampler;

			lv_imageInfos[i+1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfos[i+1].imageView = lv_gbufferNormalGPU.image.imageView;
			lv_imageInfos[i+1].sampler = lv_gbufferNormalGPU.sampler;

			lv_imageInfos[i+2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			lv_imageInfos[i+2].imageView = lv_gbufferAlbedoSpecGPU.image.imageView;
			lv_imageInfos[i+2].sampler = lv_gbufferAlbedoSpecGPU.sampler;

			lv_imageInfos[i + 3].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			lv_imageInfos[i + 3].imageView = lv_outputImage.image.imageView;
			lv_imageInfos[i + 3].sampler = lv_outputImage.sampler;

		}


		for (uint32_t i = 0; i < lv_totalNumSwapchains; ++i) {

			lv_writes[lv_totalNumBindings * i].descriptorCount = 1;
			lv_writes[lv_totalNumBindings * i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			lv_writes[lv_totalNumBindings * i].dstArrayElement = 0;
			lv_writes[lv_totalNumBindings * i].dstBinding = 0;
			lv_writes[lv_totalNumBindings * i].dstSet = m_descriptorSets[i];
			lv_writes[lv_totalNumBindings * i].pBufferInfo = &lv_bufferInfos[lv_totalNumBuffers*i];
			lv_writes[lv_totalNumBindings * i].pImageInfo = nullptr;
			lv_writes[lv_totalNumBindings * i].pNext = nullptr;
			lv_writes[lv_totalNumBindings * i].pTexelBufferView = nullptr;
			lv_writes[lv_totalNumBindings * i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[lv_totalNumBindings * i+1].descriptorCount = 1;
			lv_writes[lv_totalNumBindings * i+1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			lv_writes[lv_totalNumBindings * i+1].dstArrayElement = 0;
			lv_writes[lv_totalNumBindings * i+1].dstBinding = 1;
			lv_writes[lv_totalNumBindings * i+1].dstSet = m_descriptorSets[i];
			lv_writes[lv_totalNumBindings * i+1].pBufferInfo = &lv_bufferInfos[lv_totalNumBuffers*i + 1];
			lv_writes[lv_totalNumBindings * i+1].pImageInfo = nullptr;
			lv_writes[lv_totalNumBindings * i+1].pNext = nullptr;
			lv_writes[lv_totalNumBindings * i+1].pTexelBufferView = nullptr;
			lv_writes[lv_totalNumBindings * i+1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[lv_totalNumBindings * i+2].descriptorCount = 1;
			lv_writes[lv_totalNumBindings * i+2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			lv_writes[lv_totalNumBindings * i+2].dstArrayElement = 0;
			lv_writes[lv_totalNumBindings * i+2].dstBinding = 2;
			lv_writes[lv_totalNumBindings * i+2].dstSet = m_descriptorSets[i];
			lv_writes[lv_totalNumBindings * i+2].pBufferInfo = &lv_bufferInfos[lv_totalNumBuffers*i+2];
			lv_writes[lv_totalNumBindings * i+2].pImageInfo = nullptr;
			lv_writes[lv_totalNumBindings * i+2].pNext = nullptr;
			lv_writes[lv_totalNumBindings * i+2].pTexelBufferView = nullptr;
			lv_writes[lv_totalNumBindings * i+2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;


			lv_writes[lv_totalNumBindings * i + 3].descriptorCount = 1;
			lv_writes[lv_totalNumBindings * i + 3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[lv_totalNumBindings * i + 3].dstArrayElement = 0;
			lv_writes[lv_totalNumBindings * i+ 3].dstBinding = 3;
			lv_writes[lv_totalNumBindings * i + 3].dstSet = m_descriptorSets[i];
			lv_writes[lv_totalNumBindings * i + 3].pBufferInfo = nullptr;
			lv_writes[lv_totalNumBindings * i + 3].pImageInfo = &lv_imageInfos[lv_totalNumTextures*i];
			lv_writes[lv_totalNumBindings * i + 3].pNext = nullptr;
			lv_writes[lv_totalNumBindings * i + 3].pTexelBufferView = nullptr;
			lv_writes[lv_totalNumBindings * i + 3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[lv_totalNumBindings * i + 4].descriptorCount = 1;
			lv_writes[lv_totalNumBindings * i + 4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[lv_totalNumBindings * i + 4].dstArrayElement = 0;
			lv_writes[lv_totalNumBindings * i + 4].dstBinding = 4;
			lv_writes[lv_totalNumBindings * i + 4].dstSet = m_descriptorSets[i];
			lv_writes[lv_totalNumBindings * i + 4].pBufferInfo = nullptr;
			lv_writes[lv_totalNumBindings * i + 4].pImageInfo = &lv_imageInfos[lv_totalNumTextures*i + 1];
			lv_writes[lv_totalNumBindings * i + 4].pNext = nullptr;
			lv_writes[lv_totalNumBindings * i + 4].pTexelBufferView = nullptr;
			lv_writes[lv_totalNumBindings * i + 4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[lv_totalNumBindings * i + 5].descriptorCount = 1;
			lv_writes[lv_totalNumBindings * i + 5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			lv_writes[lv_totalNumBindings * i + 5].dstArrayElement = 0;
			lv_writes[lv_totalNumBindings * i + 5].dstBinding = 5;
			lv_writes[lv_totalNumBindings * i + 5].dstSet = m_descriptorSets[i];
			lv_writes[lv_totalNumBindings * i + 5].pBufferInfo = nullptr;
			lv_writes[lv_totalNumBindings * i + 5].pImageInfo = &lv_imageInfos[lv_totalNumTextures*i + 2];
			lv_writes[lv_totalNumBindings * i + 5].pNext = nullptr;
			lv_writes[lv_totalNumBindings * i + 5].pTexelBufferView = nullptr;
			lv_writes[lv_totalNumBindings * i + 5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			lv_writes[lv_totalNumBindings * i + 6].descriptorCount = 1;
			lv_writes[lv_totalNumBindings * i + 6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			lv_writes[lv_totalNumBindings * i + 6].dstArrayElement = 0;
			lv_writes[lv_totalNumBindings * i + 6].dstBinding = 6;
			lv_writes[lv_totalNumBindings * i + 6].dstSet = m_descriptorSets[i];
			lv_writes[lv_totalNumBindings * i + 6].pBufferInfo = nullptr;
			lv_writes[lv_totalNumBindings * i + 6].pImageInfo = &lv_imageInfos[lv_totalNumTextures * i + 3];
			lv_writes[lv_totalNumBindings * i + 6].pNext = nullptr;
			lv_writes[lv_totalNumBindings * i + 6].pTexelBufferView = nullptr;
			lv_writes[lv_totalNumBindings * i + 6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		}


		vkUpdateDescriptorSets(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device,
			lv_writes.size(), lv_writes.data(), 0, nullptr);
		

	}


	/*void DeferredComputeRenderer::CreateRenderPass()
	{

		using namespace ErrorCheck;

		
		VkAttachmentDescription lv_attachmentDescription{};
		lv_attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		lv_attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		lv_attachmentDescription.flags = 0;
		lv_attachmentDescription.format = VK_FORMAT_B8G8R8A8_UNORM;
		lv_attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		lv_attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		lv_attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		lv_attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		lv_attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		

		VkAttachmentReference lv_attachmentRef{};
		lv_attachmentRef.attachment = 0;
		lv_attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription lv_subpass{};
		lv_subpass.colorAttachmentCount = 1;
		lv_subpass.pColorAttachments = &lv_attachmentRef;
		lv_subpass.pDepthStencilAttachment = nullptr;
		lv_subpass.pInputAttachments = nullptr;
		lv_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		lv_subpass.pPreserveAttachments = nullptr;
		lv_subpass.preserveAttachmentCount = 0;
		lv_subpass.pResolveAttachments = nullptr;
		lv_subpass.flags = 0;

		VkRenderPassCreateInfo lv_renderpassCreateInfo;
		lv_renderpassCreateInfo.attachmentCount = 1;
		lv_renderpassCreateInfo.dependencyCount = 0;
		lv_renderpassCreateInfo.flags = 0;
		lv_renderpassCreateInfo.pAttachments = &lv_attachmentDescription;
		lv_renderpassCreateInfo.pDependencies = nullptr;
		lv_renderpassCreateInfo.pNext = nullptr;
		lv_renderpassCreateInfo.pSubpasses = &lv_subpass;
		lv_renderpassCreateInfo.subpassCount = 1;
		lv_renderpassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

		VULKAN_CHECK(vkCreateRenderPass(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device,
			&lv_renderpassCreateInfo, nullptr, &m_renderPass.m_renderpass));

		m_renderPass.m_info.clearColor_ = true;
		m_renderPass.m_info.clearDepth_ = false;
		m_renderPass.m_info.flags_ = 0;

		m_vulkanRenderContext.GetResourceManager().AddVulkanRenderpass(m_renderPass.m_renderpass);

	}*/


	void DeferredComputeRenderer::UpdateBuffers(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure)
	{

		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();

		memset(m_tileBuffersCPU[l_currentSwapchainIndex].data(), 0, sizeof(uint32_t)*(uint32_t)m_tileBuffersCPU[l_currentSwapchainIndex].size());


		auto& lv_uniformBufferCameraGPU = lv_vkResManager.RetrieveGpuBuffer(m_uniformBuffersCameraHandles[l_currentSwapchainIndex]);
		DeferredUniformBufferCamera lv_uniformBufferCameraCPU{};

		const float lv_ratio = (float)m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferWidth / (float)m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferHeight;

		glm::mat4 lv_mtx = glm::perspective(45.f, lv_ratio, 0.1f, 256.f) * l_cameraStructure.m_viewMatrix;

		auto lv_camPosVec3 = l_cameraStructure.m_cameraPos;
		lv_uniformBufferCameraCPU.m_cameraPos = glm::vec4{ lv_camPosVec3.x, lv_camPosVec3.y, lv_camPosVec3.z, 1.f };
		lv_uniformBufferCameraCPU.m_mvp = lv_mtx;
		lv_uniformBufferCameraCPU.scale = 1.3f;
		lv_uniformBufferCameraCPU.zNear = 0.1f;
		lv_uniformBufferCameraCPU.zFar = 256.f;
		lv_uniformBufferCameraCPU.radius = 0.2f;
		lv_uniformBufferCameraCPU.bias = 0.2f;
		lv_uniformBufferCameraCPU.distScale = 0.5f;
		lv_uniformBufferCameraCPU.attScale = 1.3f;
		lv_uniformBufferCameraCPU.m_enableDeferred = (uint32_t)1;


		m_uniformBuffersCPU[l_currentSwapchainIndex].m_lights[0].m_position = glm::vec4{ 0.f, 1.f, 2.f, 1.f };



		for (uint32_t i = 1, j = 0; i < 16; ++i, j += 3) {

			m_uniformBuffersCPU[l_currentSwapchainIndex].m_lights[i].m_position = glm::vec4{ -28.f + (float)j, 1.f, 1.5f, 1.f };
		}

		for (uint32_t i = 0, j = 0; i < 16; ++i, j += 3) {
			m_uniformBuffersCPU[l_currentSwapchainIndex].m_lights[i + 16].m_position = glm::vec4{ -28.f + (float)j, 1.f, -2.f, 1.f };
		}

		for (uint32_t i = 0, j = 0; i < 16; ++i, j += 3) {
			m_uniformBuffersCPU[l_currentSwapchainIndex].m_lights[32 + i].m_position = glm::vec4{ -28.f + (float)j, 6.f, 1.5f, 1.f };
		}

		for (uint32_t i = 0, j = 0; i < 16; ++i, j += 3) {
			m_uniformBuffersCPU[l_currentSwapchainIndex].m_lights[48 + i].m_position = glm::vec4{ -28.f + (float)j, 6.f, -2.f, 1.f };

		}

		ApplyZBinning(l_currentSwapchainIndex, l_cameraStructure);

		auto& lv_uniformBufferLightDataGPU = lv_vkResManager.RetrieveGpuBuffer(m_uniformBuffersLightDataHandles[l_currentSwapchainIndex]);
		auto& lv_storageBufferTilesGPU = lv_vkResManager.RetrieveGpuBuffer(m_tileBuffersHandles[l_currentSwapchainIndex]);

		memcpy(lv_uniformBufferLightDataGPU.ptr, &m_uniformBuffersCPU[l_currentSwapchainIndex], sizeof(UniformBuffer));
		memcpy(lv_storageBufferTilesGPU.ptr, m_tileBuffersCPU[l_currentSwapchainIndex].data(), sizeof(uint32_t) * (uint32_t)m_tileBuffersCPU[l_currentSwapchainIndex].size());
		memcpy(lv_uniformBufferCameraGPU.ptr, &lv_uniformBufferCameraCPU, sizeof(DeferredUniformBufferCamera));

	}



	void DeferredComputeRenderer::ApplyZBinning(const uint32_t l_currentSwapchainIndex, 
		const VulkanEngine::CameraStructure& l_cameraStructure)
	{
		auto& lv_vkResManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_totalNumSwapChains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();

		float lv_nearPlane = 0.1f;
		float lv_farPlane = 256.f;
		float lv_normFarNear = lv_farPlane - lv_nearPlane;

		const float lv_ratio = (float)m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferWidth / (float)m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferHeight;


		std::array<SortedLight, m_totalNumLights> lv_sortedLights;


		for (size_t i = 0; i < m_totalNumLights; ++i) {


			auto& lv_lightPos3D = m_uniformBuffersCPU[l_currentSwapchainIndex].m_lights[i].m_position;
			auto lv_radius = m_uniformBuffersCPU[l_currentSwapchainIndex].m_lights[i].m_radius;
			const glm::vec4 lv_lightPos = glm::vec4{ lv_lightPos3D.x, lv_lightPos3D.y, lv_lightPos3D.z, 1.f};

			lv_sortedLights[i].m_worldPos = lv_lightPos;
			lv_sortedLights[i].m_lightIndex = i;
			lv_sortedLights[i].m_viewPos = l_cameraStructure.m_viewMatrix * lv_lightPos;

			//The Z values are negative in view space
			lv_sortedLights[i].m_linearizedViewPosZ = (lv_sortedLights[i].m_viewPos.z - lv_nearPlane) / lv_normFarNear;

			lv_sortedLights[i].m_radius = lv_radius;

			lv_sortedLights[i].m_viewAABBMax = lv_sortedLights[i].m_viewPos + glm::vec4{ +lv_radius, lv_radius, lv_radius, 0 };
			lv_sortedLights[i].m_linearizedViewAABBMaxZ = (-lv_sortedLights[i].m_viewAABBMax.z - lv_nearPlane) / lv_normFarNear;

			lv_sortedLights[i].m_viewAABBMin = lv_sortedLights[i].m_viewPos + glm::vec4{ -lv_radius, -lv_radius, -lv_radius, 0.f };
			lv_sortedLights[i].m_linearizedViewAABBMinZ = (-lv_sortedLights[i].m_viewAABBMin.z - lv_nearPlane) / lv_normFarNear;

		}


		auto SortingLightsBasedOnZ = [](const SortedLight& l_a, const SortedLight& l_b) -> bool
			{

				return l_a.m_linearizedViewPosZ < l_b.m_linearizedViewPosZ;

			};

		std::sort(lv_sortedLights.begin(), lv_sortedLights.end(), SortingLightsBasedOnZ);

		for (size_t i = 0; i < m_totalNumLights; ++i) {

			m_uniformBuffersCPU[l_currentSwapchainIndex].m_sortedLightsIndices[i].m_value = lv_sortedLights[i].m_lightIndex;
			
		}

		for (size_t i = 0; i < m_totalNumBins; ++i) {

			uint32_t lv_maxValueInBin{ 0 };
			uint32_t lv_minValueInBin{ UINT32_MAX };

			float lv_b = (float)((float)(i+1) * (float)(1.f / (float)m_totalNumBins));
			float lv_a = (float)((i) * (float)(1.f / (float)m_totalNumBins));

			for (size_t j = 0; j < lv_sortedLights.size(); ++j) {

				auto lv_z = lv_sortedLights[j].m_viewPos.z;
				auto lv_AABBMinZ = lv_sortedLights[j].m_linearizedViewAABBMinZ;
				auto lv_AABBMaxZ = lv_sortedLights[j].m_linearizedViewAABBMaxZ;

				if (0 < lv_z && lv_z < 1) {
					if (lv_a <= lv_z && lv_z <= lv_b ||
						lv_a <= lv_AABBMaxZ && lv_AABBMaxZ <= lv_b ||
						lv_a <= lv_AABBMinZ && lv_AABBMinZ <= lv_b) {

						if (j < lv_minValueInBin) {
							lv_minValueInBin = (uint32_t)j;
						}

						if (lv_maxValueInBin < j) {
							lv_maxValueInBin = (uint32_t)j;
						}

					}
				}

			}

			m_uniformBuffersCPU[l_currentSwapchainIndex].m_bins[i].m_value = lv_minValueInBin | (lv_maxValueInBin << 16);
		}
		

		auto lv_resolWidth{ m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferWidth };
		auto lv_resolHeight{ m_vulkanRenderContext.GetContextCreator().m_vkDev.m_framebufferHeight };


		for (auto& l_sortedLight : lv_sortedLights) {

			auto lv_projectionMatrix = glm::perspective(45.f, lv_ratio, 0.1f, 256.f);
			auto lv_radius = l_sortedLight.m_radius;
			auto& lv_worldPosLight = l_sortedLight.m_worldPos;

			if (l_sortedLight.m_linearizedViewPosZ <= 0 || l_sortedLight.m_linearizedViewPosZ >= 1) { continue; }

			/*l_sortedLight.m_viewAABBMin = lv_projectionMatrix* l_sortedLight.m_viewAABBMin;
			l_sortedLight.m_viewAABBMin = (1.f / l_sortedLight.m_viewAABBMin.w) * (l_sortedLight.m_viewAABBMin);

			l_sortedLight.m_viewAABBMax = lv_projectionMatrix * l_sortedLight.m_viewAABBMax;
			l_sortedLight.m_viewAABBMax = (1.f / l_sortedLight.m_viewAABBMax.w) * (l_sortedLight.m_viewAABBMax);*/

			glm::vec4 lv_aabb2D{
				FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX
				/*l_sortedLight.m_viewAABBMin.x, -1.f*l_sortedLight.m_viewAABBMin.y,
			l_sortedLight.m_viewAABBMax.x, -1.f*l_sortedLight.m_viewAABBMax.y*/};

			for (uint32_t i = 0; i < 8; ++i) {
				glm::vec3 corner{ (i % 2) ? 1.f : -1.f, (i & 2) ?
					1.f : -1.f, (i & 4) ? 1.f : -1.f};
				corner = lv_radius * corner;
				auto cornerNew = glm::vec3{ lv_worldPosLight.x, lv_worldPosLight.y, lv_worldPosLight.z };
				glm::vec4 corner_vs = l_cameraStructure.m_viewMatrix * glm::vec4{ cornerNew, 1.f};
				corner_vs += glm::vec4{ corner , 0.f};
				corner_vs.z = std::max(corner_vs.z, lv_nearPlane);
				glm::vec4 corner_ndc = lv_projectionMatrix * corner_vs;
				corner_ndc = (1.f / corner_ndc.w) * corner_ndc;
				lv_aabb2D.x = std::min(lv_aabb2D.x, corner_ndc.x);
				lv_aabb2D.y = std::min(lv_aabb2D.y, corner_ndc.y);
				lv_aabb2D.z = std::max(lv_aabb2D.z, corner_ndc.x);
				lv_aabb2D.w = std::max(lv_aabb2D.w, corner_ndc.y);

				/*corner_vs.z = -std::max(-corner_vs.z, lv_nearPlane);
				glm::vec4 corner_ndc = lv_projectionMatrix * corner_vs;
				corner_ndc = (1.f/corner_ndc.w) * corner_ndc;
				lv_aabb2D.x = std::min(lv_aabb2D.x, corner_ndc.x);
				lv_aabb2D.y = std::min(lv_aabb2D.y, corner_ndc.y);
				lv_aabb2D.z = std::max(lv_aabb2D.z, corner_ndc.x);
				lv_aabb2D.w = std::max(lv_aabb2D.w, corner_ndc.y);*/
			}
			
			//std::swap(lv_aabb2D.y, lv_aabb2D.w);
			/*lv_aabb2D.y *= -1.f;
			lv_aabb2D.w *= -1.f;
			std::swap(lv_aabb2D.y, lv_aabb2D.w);*/

			lv_aabb2D.x = (lv_aabb2D.x * 0.5f + 0.5f) * (float)(lv_resolWidth-1);
			lv_aabb2D.z = (lv_aabb2D.z * 0.5f + 0.5f) * (float)(lv_resolWidth - 1);
			lv_aabb2D.y = (lv_aabb2D.y * 0.5f - 0.5f) * (float)(lv_resolHeight - 1);
			lv_aabb2D.w = (lv_aabb2D.w * 0.5f - 0.5f) * (float)(lv_resolHeight - 1);

			/*printf("\n---------\n");
			printf("\nMin point: (%f, %f)\n", lv_aabb2D.x, lv_aabb2D.y);
			printf("\nMax point: (%f, %f)\n", lv_aabb2D.z, lv_aabb2D.w);
			printf("\n---------\n");*/


			auto lv_width = lv_aabb2D.z - lv_aabb2D.x;
			auto lv_height = lv_aabb2D.w - lv_aabb2D.y;

			if (lv_width < 0.0001f && lv_height < 0.0001f) {
				continue;
			}

			if (lv_aabb2D.x >= lv_resolWidth - 1 || lv_aabb2D.y >= lv_resolHeight - 1) {
				continue;
			}

			if (lv_aabb2D.z <= 0 || lv_aabb2D.w <= 0) {
				continue;
			}

			lv_aabb2D.x = std::max(0.f, lv_aabb2D.x);
			lv_aabb2D.y = std::max(0.f, lv_aabb2D.y);

			lv_aabb2D.z = std::min(lv_aabb2D.z, (float)(lv_resolWidth - 1));
			lv_aabb2D.w = std::min(lv_aabb2D.w, (float)(lv_resolHeight - 1));

			uint32_t lv_firstTileX = (uint32_t)(lv_aabb2D.x / 8.f);
			uint32_t lv_firstTileY = (uint32_t)(lv_aabb2D.y / 8.f);

			uint32_t lv_lastTileX = (uint32_t)(lv_aabb2D.z/8.f);
			uint32_t lv_lastTileY = (uint32_t)(lv_aabb2D.w / 8.f);

			for (uint32_t i = lv_firstTileY; i <= lv_lastTileY; ++i) {
				for (uint32_t j = lv_firstTileX; j <= lv_lastTileX; ++j) {

					uint32_t lv_indexTile = lv_resolWidth * i + j*8;

					uint32_t lv_rowNumInTile =(uint32_t) (l_sortedLight.m_lightIndex / 32.f);
					uint32_t lv_bitIndex = l_sortedLight.m_lightIndex % 32;

					m_tileBuffersCPU[l_currentSwapchainIndex][lv_indexTile + lv_rowNumInTile] |= (1 << lv_bitIndex);
				}
			}
		}

		

	}






}