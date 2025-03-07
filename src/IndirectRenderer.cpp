


#include "IndirectRenderer.hpp"
#include <array>
#include "ErrorCheck.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <format>
#include <string>
#include "CameraStructure.hpp"
#include "UtilsMath.h"


namespace RenderCore
{


	IndirectRenderer::IndirectRenderer(VulkanEngine::VulkanRenderContext& l_vkRenderContext,
		VkCommandBuffer l_cmdBuffer,
		const char* l_meshHeaderFile,
		const char* l_boundingBoxFile,
		const char* l_instanceDataFile,
		const char* l_materialFile,
		const char* l_sceneFile,
		const char* l_vtxShaderFile,
		const char* l_fragShaderFile,
		const std::string& l_spirvFile):
		Renderbase(l_vkRenderContext),
		m_materialLoaderSaver(l_materialFile),
		m_sceneLoaderSaver(l_sceneFile)
	{
		
		auto& lv_contextCreator = m_vulkanRenderContext.GetContextCreator();
		auto& lv_vulkanResourceManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();
		auto& lv_frameGraph = m_vulkanRenderContext.GetFrameGraph();

		const float lv_ratio = (float)lv_contextCreator.m_vkDev.m_framebufferWidth / (float)lv_contextCreator.m_vkDev.m_framebufferHeight;
		m_cameraFrustum.m_projectionMatrix = glm::perspective((float)glm::radians(60.f), lv_ratio, 0.01f, 1000.f);

		LoadInstanceData(l_instanceDataFile);
		m_instanceBufferSize = m_totalNumInstances * sizeof(InstanceData);
		m_materialLoaderSaver.LoadMaterialFile();
		
		LoadAllTexturesOfScene();
		LoadBoundingBoxData(l_boundingBoxFile);

		m_vulkanRenderContext.GetCpuResourceProvider().AddCpuResource("BoundingBoxData"
			, (void*)m_boundingBoxes.data(), (uint32_t)m_boundingBoxes.size());

		auto lv_meshHeader = LoadMeshData(l_meshHeaderFile);

		if (0x12345678 != lv_meshHeader.m_magicValue) {
			printf("Mesh header file is corrupted.\n");
			exit(EXIT_FAILURE);
		}
		m_vertexBufferSize = lv_meshHeader.m_vertexDataSize;
		m_indexBufferSize = lv_meshHeader.m_lodDataSize;

		m_materialBufferSize =(uint32_t) (m_materialLoaderSaver.GetMaterials().size()*sizeof(SceneConverter::Material));
		
		for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
			m_uniformBuffers.push_back(lv_vulkanResourceManager.CreateBuffer(sizeof(IndirectUniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				std::format("IndirectUniformBuffer {} ", i).c_str()));
		}

		auto lv_totalNumSwapchainImages = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();
		
		m_materialBufferHandle = lv_vulkanResourceManager.CreateBufferWithHandle(m_materialBufferSize, 
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, " Material-Buffer-Indirect ");
		lv_vulkanResourceManager.AddGpuResource(" Material-Buffer-Indirect ", m_materialBufferHandle,
			VulkanResourceManager::VulkanDataType::m_buffer);

		UpdateLocalDeviceBuffers(l_cmdBuffer, m_materialBufferHandle, m_materialLoaderSaver.GetMaterials().data());

		VkPhysicalDeviceProperties lv_devProps;
		vkGetPhysicalDeviceProperties(lv_contextCreator.m_vkDev.m_physicalDevice,
			&lv_devProps);
		const uint32_t lv_offsetAlignment = lv_devProps.limits.minStorageBufferOffsetAlignment;


		if ((m_vertexBufferSize & (lv_offsetAlignment - 1)) != 0)
		{
			int floats = (lv_offsetAlignment - (m_vertexBufferSize & (lv_offsetAlignment - 1))) / sizeof(float);

			for (int ii = 0; ii < floats; ii++) {
				m_vertexBuffers.push_back(0);
			}

			m_vertexBufferSize = (m_vertexBufferSize + lv_offsetAlignment) & ~(lv_offsetAlignment - 1);
		}


		{
			m_arrayTexturesHandles.resize(m_materialLoaderSaver.GetFileNames().size());

			for (size_t i = 0; i < m_arrayTexturesHandles.size(); ++i) {
				m_arrayTexturesHandles[i] = m_textureHandlesOfScene[i];
			}
		}



		{


			m_vertexDataBufferHandle = m_vulkanRenderContext.GetResourceManager()
				.CreateBufferWithHandle(m_vertexBufferSize + m_indexBufferSize, 
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, " Vertex-Buffer-Indirect ");

			m_indicesDataBufferHandle = m_vertexDataBufferHandle;
		}

		UpdateGeometryBuffers(l_cmdBuffer);

		{
			m_transformationsBufferHandles.resize(lv_contextCreator.m_vkDev.m_swapchainImages.size());

			for (uint32_t i = 0; i < lv_contextCreator.m_vkDev.m_swapchainImages.size(); ++i) {
				m_transformationsBufferHandles[i] = m_vulkanRenderContext.GetResourceManager()
					.CreateBufferWithHandle(sizeof(glm::mat4) * m_sceneLoaderSaver.GetScene().m_globalTransforms.size(), 
						VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
						VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
						VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, " Transformation-matrices-Buffer-Indirect ");

				UpdateTransformationsBuffer(i);
			}
		}

		m_instanceBuffersGpu.resize(lv_contextCreator.m_vkDev.m_swapchainImages.size());
		m_indirectBufferHandles.resize(lv_contextCreator.m_vkDev.m_swapchainImages.size());

		for (uint32_t i = 0; i < lv_contextCreator.m_vkDev.m_swapchainImages.size(); ++i) {


			m_indirectBufferHandles[i] = m_vulkanRenderContext.GetResourceManager()
				.CreateBufferWithHandle(sizeof(VkDrawIndirectCommand) * m_totalNumInstances,
					VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					std::format("Indirect-Buffer-Shader-Indirect {}", i).c_str());

			UpdateIndirectBuffer(i);


			m_instanceBuffersGpu[i]= m_vulkanRenderContext.GetResourceManager()
				.CreateBufferWithHandle(m_instanceBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					std::format("Instance-Buffer-Indirect {}", i).c_str());

			UpdateInstanceBuffer(i);

		}

		GeneratePipelineFromSpirvBinaries(l_spirvFile);
		SetRenderPassAndFrameBuffer("IndirectGbuffer");

		SetNodeToAppropriateRenderpass("IndirectGbuffer", this);

		auto* lv_node = lv_frameGraph.RetrieveNode("IndirectGbuffer");

		m_attachmentHandles.resize(lv_node->m_inputResourcesHandles.size() * lv_totalNumSwapchainImages);

		for (size_t i = 0, j = 0; i < lv_node->m_inputResourcesHandles.size() *lv_totalNumSwapchainImages; i+= lv_node->m_inputResourcesHandles.size(), ++j) {
			
			auto lv_formattedArg = std::make_format_args(j);

			std::string lv_formattedString{"GBufferTangent {}"};
			auto lv_colorMeta = lv_vulkanResourceManager.RetrieveGpuResourceMetaData(std::vformat(lv_formattedString,lv_formattedArg));

			lv_formattedString = "GBufferPosition {}";
			auto lv_posMeta = lv_vulkanResourceManager.RetrieveGpuResourceMetaData(std::vformat(lv_formattedString, lv_formattedArg));

			lv_formattedString = "GBufferNormal {}";
			auto lv_normalMeta = lv_vulkanResourceManager.RetrieveGpuResourceMetaData(std::vformat(lv_formattedString, lv_formattedArg));

			lv_formattedString = "GBufferAlbedoSpec {}";
			auto lv_albedoSpecMeta = lv_vulkanResourceManager.RetrieveGpuResourceMetaData(std::vformat(lv_formattedString, lv_formattedArg));

			lv_formattedString = "GBufferNormalVertex {}";
			auto lv_normalVertexMeta = lv_vulkanResourceManager.RetrieveGpuResourceMetaData(std::vformat(lv_formattedString, lv_formattedArg));

			lv_formattedString = "GBufferMetallic {}";
			auto lv_metallicMeta = lv_vulkanResourceManager.RetrieveGpuResourceMetaData(std::vformat(lv_formattedString, lv_formattedArg));

			lv_formattedString = "Depth {}";
			auto lv_depthMeta = lv_vulkanResourceManager.RetrieveGpuResourceMetaData(std::vformat(lv_formattedString, lv_formattedArg));

			m_attachmentHandles[i] = lv_colorMeta.m_resourceHandle;
			m_attachmentHandles[i+1] = lv_posMeta.m_resourceHandle;
			m_attachmentHandles[i+2] = lv_normalMeta.m_resourceHandle;
			m_attachmentHandles[i+3] = lv_albedoSpecMeta.m_resourceHandle;
			m_attachmentHandles[i+4] = lv_normalVertexMeta.m_resourceHandle;
			m_attachmentHandles[i + 5] = lv_metallicMeta.m_resourceHandle;
			m_attachmentHandles[i + 6] = lv_depthMeta.m_resourceHandle;
		}



		//CreateRenderPass();
		//CreateFramebuffers();
		UpdateDescriptorSets();

		lv_frameGraph.IncrementNumNodesPerCmdBuffer(0);

		m_pipelineLayout = m_vulkanRenderContext.GetResourceManager()
			.CreatePipelineLayout(m_descriptorSetLayout, " Pipeline-Layout-Indirect ");

		VulkanResourceManager::PipelineInfo lv_pipelineInfo{};
		lv_pipelineInfo.m_dynamicScissorState = false;
		lv_pipelineInfo.m_height = lv_contextCreator.m_vkDev.m_framebufferHeight;
		lv_pipelineInfo.m_width = lv_contextCreator.m_vkDev.m_framebufferWidth;
		lv_pipelineInfo.m_useBlending = false;
		lv_pipelineInfo.m_useDepth = true;
		lv_pipelineInfo.m_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		lv_pipelineInfo.m_totalNumColorAttach = ((uint32_t)lv_node->m_outputResourcesHandles.size())-1;

		m_graphicsPipeline = m_vulkanRenderContext.GetResourceManager()
			.CreateGraphicsPipeline(m_renderPass, m_pipelineLayout,
				{ l_vtxShaderFile, l_fragShaderFile }, " Graphics-Pipeline-Indirect ", lv_pipelineInfo);
	}


	void IndirectRenderer::LoadAllTexturesOfScene()
	{
		auto& lv_alltextureFileNames = m_materialLoaderSaver.GetFileNames();

		m_textureHandlesOfScene.resize(lv_alltextureFileNames.size());

		for (size_t i = 0; i < lv_alltextureFileNames.size(); ++i) {
			m_textureHandlesOfScene[i] = m_vulkanRenderContext.GetResourceManager().LoadTexture2DWithHandle(lv_alltextureFileNames[i]);
		}
		
	}



	/*void IndirectRenderer::CreateFramebuffers()
	{

		auto& lv_vulkanResourceManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_totalNumSwapchainImages = (uint32_t)m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();

		uint32_t lv_totalNumAttachmentsPerFrame{ 5 };

		m_attachmentHandles.resize(lv_totalNumAttachmentsPerFrame * lv_totalNumSwapchainImages);
		m_framebufferHandles.resize(lv_totalNumSwapchainImages);

		for (uint32_t i = 0, j = 0; 
			i < (uint32_t)(m_attachmentHandles.size()) && j < lv_totalNumSwapchainImages; 
			++j,i += lv_totalNumAttachmentsPerFrame) {


			m_attachmentHandles[i] = lv_vulkanResourceManager.CreateTexture(std::format("colorAttachmentIndirect{}", j).c_str(),
				VK_FORMAT_R8G8B8A8_UNORM);
			m_attachmentHandles[i + 1] = lv_vulkanResourceManager.CreateTexture(std::format("gBufferPosition{}", j).c_str(),
				VK_FORMAT_R32G32B32A32_SFLOAT);
			m_attachmentHandles[i + 2] = lv_vulkanResourceManager.CreateTexture(std::format("gBufferNormal{}", j).c_str(),
				VK_FORMAT_R32G32B32A32_SFLOAT);
			m_attachmentHandles[i + 3] = lv_vulkanResourceManager.CreateTexture(std::format("gBufferAlbedoSpec{}", j).c_str(),
				VK_FORMAT_R8G8B8A8_UNORM);
			m_attachmentHandles[i + 4] = lv_vulkanResourceManager.CreateDepthTextureWithHandle(std::format("depthAttachmentIndirect{}", j).c_str());


			lv_vulkanResourceManager.AddGpuResource(std::format("colorAttachmentIndirect{}", j).c_str(),
				m_attachmentHandles[i],
				VulkanResourceManager::VulkanDataType::m_texture);
			lv_vulkanResourceManager.AddGpuResource(std::format("gBufferPosition{}", j).c_str(), m_attachmentHandles[i + 1],
				VulkanResourceManager::VulkanDataType::m_texture);
			lv_vulkanResourceManager.AddGpuResource(std::format("gBufferNormal{}", j).c_str(), m_attachmentHandles[i + 2],
				VulkanResourceManager::VulkanDataType::m_texture);
			lv_vulkanResourceManager.AddGpuResource(std::format("gBufferAlbedoSpec{}", j).c_str(), m_attachmentHandles[i + 3],
				VulkanResourceManager::VulkanDataType::m_texture);
			lv_vulkanResourceManager.AddGpuResource(std::format("depthAttachmentIndirect{}", j).c_str(),
				m_attachmentHandles[i + 4], VulkanResourceManager::VulkanDataType::m_texture);

		}


		for (size_t i = 0; i < lv_totalNumSwapchainImages; ++i) {

			m_framebufferHandles[i] = lv_vulkanResourceManager.CreateFrameBuffer(m_renderPass,
				std::vector<uint32_t>{m_attachmentHandles[lv_totalNumAttachmentsPerFrame * i],
				m_attachmentHandles[lv_totalNumAttachmentsPerFrame * i + 1],
				m_attachmentHandles[lv_totalNumAttachmentsPerFrame * i + 2],
				m_attachmentHandles[lv_totalNumAttachmentsPerFrame * i + 3],
				m_attachmentHandles[lv_totalNumAttachmentsPerFrame * i + 4]},
				std::format(" Framebuffer-Indirect {} ", i).c_str());
		}
	}*/



	void IndirectRenderer::UpdateBuffers(const uint32_t l_currentSwapchainIndex,
		const VulkanEngine::CameraStructure& l_cameraStructure)
	{
		auto& lv_indirectBufferGpu = m_vulkanRenderContext.GetResourceManager().RetrieveGpuBuffer(m_indirectBufferHandles[l_currentSwapchainIndex]);

		IndirectUniformBuffer lv_uniformBuffer{};

		glm::mat4 lv_mtx = l_cameraStructure.m_projectionMatrix * l_cameraStructure.m_viewMatrix;

		auto lv_camPosVec3 = l_cameraStructure.m_cameraPos;
		lv_uniformBuffer.m_cameraPos = glm::vec4{ lv_camPosVec3.x, lv_camPosVec3.y, lv_camPosVec3.z, 1.f };
		lv_uniformBuffer.m_mvp = lv_mtx;
		lv_uniformBuffer.scale = 1.3f;
		lv_uniformBuffer.zNear = 0.1f;
		lv_uniformBuffer.zFar = 256.f;
		lv_uniformBuffer.radius = 0.2f;
		lv_uniformBuffer.bias = 0.2f;
		lv_uniformBuffer.distScale = 0.5f;
		lv_uniformBuffer.attScale = 1.3f;
		lv_uniformBuffer.m_enableDeferred = (uint32_t)1;


		m_cameraFrustum.m_viewMatrix = l_cameraStructure.m_viewMatrix;
		getFrustumCorners(lv_mtx, m_cameraFrustum.m_debugViewFrustumCorners);
		getFrustumPlanes(lv_mtx, m_cameraFrustum.m_debugViewFrustumPlanes);
		UpdateIndirectBuffer(l_currentSwapchainIndex);
		UpdateInstanceBuffer(l_currentSwapchainIndex);
		UpdateTransformationsBuffer(l_currentSwapchainIndex);

		memcpy(m_uniformBuffers[l_currentSwapchainIndex].ptr, &lv_uniformBuffer, sizeof(IndirectUniformBuffer));
	}



	void IndirectRenderer::UpdateStorageBuffers(uint32_t l_currentSwapchainIndex)
	{
		
	}

	void IndirectRenderer::UpdateTransformationsBuffer(uint32_t l_currentSwapchainIndex)
	{
		auto& lv_transformationBuffer = m_vulkanRenderContext.GetResourceManager()
			.RetrieveGpuBuffer(m_transformationsBufferHandles[l_currentSwapchainIndex]);

		memcpy(lv_transformationBuffer.ptr,
			m_sceneLoaderSaver.GetScene().m_globalTransforms.data(), 
			sizeof(glm::mat4)* m_sceneLoaderSaver.GetScene().m_globalTransforms.size());
	}

	void IndirectRenderer::UpdateInstanceBuffer(uint32_t l_currentSwapchainIndex)
	{
		auto& lv_instanceBuffer = m_vulkanRenderContext.GetResourceManager()
			.RetrieveGpuBuffer(m_instanceBuffersGpu[l_currentSwapchainIndex]);

		memcpy(lv_instanceBuffer.ptr, m_outputInstanceData.data(),
			m_instanceBufferSize);
	}


	void IndirectRenderer::UpdateLocalDeviceBuffers(VkCommandBuffer l_cmdBuffer,
		const uint32_t l_bufferHandle, const void* l_dstBufferData)
	{
		using namespace ErrorCheck;
		auto& lv_contextCreator = m_vulkanRenderContext.GetContextCreator();
		auto& lv_vulkanResourceManager = m_vulkanRenderContext.GetResourceManager();

		
		auto& lv_buffer = lv_vulkanResourceManager.RetrieveGpuBuffer(l_bufferHandle);

		auto& lv_stagingBuffer = lv_vulkanResourceManager.CreateBuffer(lv_buffer.size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			, " Staging-Buffer-Indirect ");

		
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

		VULKAN_CHECK(vkQueueSubmit(lv_contextCreator.m_vkDev.m_mainQueue1, 1, &lv_submitInfo, nullptr));

		vkQueueWaitIdle(lv_contextCreator.m_vkDev.m_mainQueue1);
	}

	void IndirectRenderer::UpdateGeometryBuffers(VkCommandBuffer l_cmdBuffer)
	{
		using namespace ErrorCheck;
		auto& lv_contextCreator = m_vulkanRenderContext.GetContextCreator();
		auto& lv_vulkanResourceManager = m_vulkanRenderContext.GetResourceManager();
		

		auto& lv_stagingBuffer = lv_vulkanResourceManager.CreateBuffer(m_vertexBufferSize + m_indexBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT 
			| VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		, " Staging-Buffer-Indirect ");


		memcpy(lv_stagingBuffer.ptr, m_vertexBuffers.data(), m_vertexBufferSize);
		vkUnmapMemory(lv_contextCreator.m_vkDev.m_device, lv_stagingBuffer.memory);
		


		uploadBufferData(lv_contextCreator.m_vkDev, lv_stagingBuffer.memory,
			m_vertexBufferSize, m_indexBuffers.data(), m_indexBufferSize);


		auto& lv_vertexBuffer = m_vulkanRenderContext.GetResourceManager().RetrieveGpuBuffer(m_vertexDataBufferHandle);

		VkBufferCopy lv_bufferCopy = {};
		lv_bufferCopy.size = m_vertexBufferSize + m_indexBufferSize;

		VkCommandBufferBeginInfo lv_commandBufferBegin = {};
		lv_commandBufferBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		lv_commandBufferBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VULKAN_CHECK(vkBeginCommandBuffer(l_cmdBuffer, &lv_commandBufferBegin));

		vkCmdCopyBuffer(l_cmdBuffer, lv_stagingBuffer.buffer,
			lv_vertexBuffer.buffer,
			1, &lv_bufferCopy);

		vkEndCommandBuffer(l_cmdBuffer);

		VkSubmitInfo lv_submitInfo = {};
		lv_submitInfo.commandBufferCount = 1;
		lv_submitInfo.pCommandBuffers = &l_cmdBuffer;
		lv_submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VULKAN_CHECK(vkQueueSubmit(lv_contextCreator.m_vkDev.m_mainQueue1, 1, &lv_submitInfo, nullptr));

		vkQueueWaitIdle(lv_contextCreator.m_vkDev.m_mainQueue1);
	}

	void IndirectRenderer::UpdateIndirectBuffer(uint32_t l_currentSwapchainIndex)
	{
		auto& lv_indirectBuffer = m_vulkanRenderContext.GetResourceManager().RetrieveGpuBuffer(m_indirectBufferHandles[l_currentSwapchainIndex]);
		VkDrawIndirectCommand* lv_drawStructure = (VkDrawIndirectCommand*)lv_indirectBuffer.ptr;

		m_totalNumVisibleMeshes = 0;

		for (uint32_t i = 0; i < m_outputInstanceData.size(); ++i) {
			auto j = m_outputInstanceData[i].m_meshIndex;
			lv_drawStructure[i].vertexCount = m_meshes[j].CalculateLODNumberOfIndices(m_outputInstanceData[i].m_lod);
			lv_drawStructure[i].firstInstance = i;
			lv_drawStructure[i].firstVertex = 0U;
			lv_drawStructure[i].instanceCount = (true == isBoxInFrustum
			(m_cameraFrustum.m_debugViewFrustumPlanes, 
				m_cameraFrustum.m_debugViewFrustumCorners, m_boundingBoxes[j])) ? 1 : 0;

			m_totalNumVisibleMeshes += lv_drawStructure[i].instanceCount;
		}

	}



	const std::vector<InstanceData>& IndirectRenderer::GetInstanceData() const
	{
		return m_outputInstanceData;
	}
	const std::vector<MeshConverter::Mesh>& IndirectRenderer::GetMeshData() const
	{
		return m_meshes;
	}



	void IndirectRenderer::FillCommandBuffer(
		VkCommandBuffer l_commandBuffer,
		uint32_t l_currentSwapchainIndex)
	{

		auto& lv_vulkanResourceManager = m_vulkanRenderContext.GetResourceManager();
		auto lv_framebuffer = lv_vulkanResourceManager.RetrieveGpuFramebuffer(m_framebufferHandles[l_currentSwapchainIndex]);
		auto& lv_indirectBuffer = lv_vulkanResourceManager.RetrieveGpuBuffer(m_indirectBufferHandles[l_currentSwapchainIndex]);


		uint32_t lv_totalNumAttachmentsPerFrameBuffer = 7;

		auto& lv_tangentAttach = lv_vulkanResourceManager
			.RetrieveGpuTexture(m_attachmentHandles[lv_totalNumAttachmentsPerFrameBuffer * l_currentSwapchainIndex]);
		auto& lv_posColorAttach = lv_vulkanResourceManager
			.RetrieveGpuTexture(m_attachmentHandles[lv_totalNumAttachmentsPerFrameBuffer * l_currentSwapchainIndex + 1]);
		auto& lv_normalColorAttach = lv_vulkanResourceManager
			.RetrieveGpuTexture(m_attachmentHandles[lv_totalNumAttachmentsPerFrameBuffer * l_currentSwapchainIndex + 2]);
		auto& lv_albedoSpecColorAttach = lv_vulkanResourceManager
			.RetrieveGpuTexture(m_attachmentHandles[lv_totalNumAttachmentsPerFrameBuffer * l_currentSwapchainIndex + 3]);
		auto& lv_normalVertexColorAttach = lv_vulkanResourceManager
			.RetrieveGpuTexture(m_attachmentHandles[lv_totalNumAttachmentsPerFrameBuffer * l_currentSwapchainIndex + 4]);
		auto& lv_metallicColorAttach = lv_vulkanResourceManager
			.RetrieveGpuTexture(m_attachmentHandles[lv_totalNumAttachmentsPerFrameBuffer * l_currentSwapchainIndex + 5]);
		auto& lv_depthAttach = lv_vulkanResourceManager
			.RetrieveGpuTexture(m_attachmentHandles[lv_totalNumAttachmentsPerFrameBuffer * l_currentSwapchainIndex + 6]);


		transitionImageLayoutCmd(l_commandBuffer, lv_tangentAttach.image.image, lv_tangentAttach.format,
			lv_tangentAttach.Layout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		transitionImageLayoutCmd(l_commandBuffer, lv_posColorAttach.image.image, lv_posColorAttach.format,
			lv_posColorAttach.Layout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		transitionImageLayoutCmd(l_commandBuffer, lv_normalColorAttach.image.image, lv_normalColorAttach.format,
			lv_normalColorAttach.Layout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		transitionImageLayoutCmd(l_commandBuffer, lv_albedoSpecColorAttach.image.image, lv_albedoSpecColorAttach.format,
			lv_albedoSpecColorAttach.Layout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		transitionImageLayoutCmd(l_commandBuffer, lv_normalVertexColorAttach.image.image, lv_normalVertexColorAttach.format,
			lv_normalVertexColorAttach.Layout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		transitionImageLayoutCmd(l_commandBuffer, lv_metallicColorAttach.image.image, lv_metallicColorAttach.format,
			lv_metallicColorAttach.Layout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		transitionImageLayoutCmd(l_commandBuffer, lv_depthAttach.image.image, lv_depthAttach.format,
			lv_depthAttach.Layout, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		lv_tangentAttach.Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		lv_posColorAttach.Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		lv_normalColorAttach.Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		lv_albedoSpecColorAttach.Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		lv_normalVertexColorAttach.Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		lv_metallicColorAttach.Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		lv_depthAttach.Layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		

		BeginRenderPass(m_renderPass, lv_framebuffer, l_commandBuffer, l_currentSwapchainIndex, m_attachmentHandles.size());
		vkCmdDrawIndirect(l_commandBuffer, lv_indirectBuffer.buffer, 0, m_totalNumInstances,
			sizeof(VkDrawIndirectCommand));
		vkCmdEndRenderPass(l_commandBuffer);

		lv_tangentAttach.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		lv_posColorAttach.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		lv_normalColorAttach.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		lv_albedoSpecColorAttach.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		lv_normalVertexColorAttach.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		lv_metallicColorAttach.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		lv_depthAttach.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	}



	/*void IndirectRenderer::CreateRenderPass()
	{


		using namespace ErrorCheck;

		constexpr uint32_t lv_totalNumAttachments{ 5 };


		std::array<VkAttachmentDescription, lv_totalNumAttachments> lv_attachmentDescriptions{};


		lv_attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		lv_attachmentDescriptions[0].format = VK_FORMAT_R8G8B8A8_UNORM;
		lv_attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		lv_attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		lv_attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		lv_attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		lv_attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		lv_attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
		lv_attachmentDescriptions[0].flags = 0;

		lv_attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		lv_attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		lv_attachmentDescriptions[1].flags = 0;
		lv_attachmentDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		lv_attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		lv_attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
		lv_attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		lv_attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		lv_attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		
		lv_attachmentDescriptions[2].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		lv_attachmentDescriptions[2].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		lv_attachmentDescriptions[2].flags = 0;
		lv_attachmentDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		lv_attachmentDescriptions[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		lv_attachmentDescriptions[2].samples = VK_SAMPLE_COUNT_1_BIT;
		lv_attachmentDescriptions[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		lv_attachmentDescriptions[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		lv_attachmentDescriptions[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		lv_attachmentDescriptions[3].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		lv_attachmentDescriptions[3].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		lv_attachmentDescriptions[3].flags = 0;
		lv_attachmentDescriptions[3].format = VK_FORMAT_R8G8B8A8_UNORM;
		lv_attachmentDescriptions[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		lv_attachmentDescriptions[3].samples = VK_SAMPLE_COUNT_1_BIT;
		lv_attachmentDescriptions[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		lv_attachmentDescriptions[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		lv_attachmentDescriptions[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;


		lv_attachmentDescriptions[4].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		lv_attachmentDescriptions[4].format = findDepthFormat(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_physicalDevice);
		lv_attachmentDescriptions[4].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		lv_attachmentDescriptions[4].flags = 0;
		lv_attachmentDescriptions[4].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		lv_attachmentDescriptions[4].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		lv_attachmentDescriptions[4].samples = VK_SAMPLE_COUNT_1_BIT;
		lv_attachmentDescriptions[4].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		lv_attachmentDescriptions[4].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		


		std::array<VkAttachmentReference, lv_totalNumAttachments> lv_attachmentRefs{};

		lv_attachmentRefs[0].attachment = 0;
		lv_attachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		lv_attachmentRefs[1].attachment = 1;
		lv_attachmentRefs[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		lv_attachmentRefs[2].attachment = 2;
		lv_attachmentRefs[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		lv_attachmentRefs[3].attachment = 3;
		lv_attachmentRefs[3].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		lv_attachmentRefs[4].attachment = 4;
		lv_attachmentRefs[4].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


		std::array<VkSubpassDescription, 1> lv_subpassDescriptions{};

		lv_subpassDescriptions[0].colorAttachmentCount = lv_attachmentRefs.size()-1;
		lv_subpassDescriptions[0].inputAttachmentCount = 0;
		lv_subpassDescriptions[0].pColorAttachments = lv_attachmentRefs.data();
		lv_subpassDescriptions[0].pDepthStencilAttachment = lv_attachmentRefs.data() + 4;
		lv_subpassDescriptions[0].pInputAttachments = nullptr;
		lv_subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		lv_subpassDescriptions[0].pPreserveAttachments = nullptr;
		lv_subpassDescriptions[0].preserveAttachmentCount = 0;
		lv_subpassDescriptions[0].pResolveAttachments = nullptr;
		lv_subpassDescriptions[0].flags = 0;

		


		std::array<VkSubpassDependency, 1> lv_subpassDeps{};

		lv_subpassDeps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		lv_subpassDeps[0].dstSubpass = 0;
		lv_subpassDeps[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		lv_subpassDeps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		lv_subpassDeps[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		lv_subpassDeps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		lv_subpassDeps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;


		VkRenderPassCreateInfo lv_renderpassCreateInfo{};
		lv_renderpassCreateInfo.attachmentCount = lv_attachmentDescriptions.size();
		lv_renderpassCreateInfo.dependencyCount = lv_subpassDeps.size();
		lv_renderpassCreateInfo.flags = 0;
		lv_renderpassCreateInfo.pAttachments = lv_attachmentDescriptions.data();
		lv_renderpassCreateInfo.pDependencies = lv_subpassDeps.data();
		lv_renderpassCreateInfo.pNext = nullptr;
		lv_renderpassCreateInfo.pSubpasses = lv_subpassDescriptions.data();
		lv_renderpassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		lv_renderpassCreateInfo.subpassCount = lv_subpassDescriptions.size();


		VULKAN_CHECK(vkCreateRenderPass(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device,
			&lv_renderpassCreateInfo, nullptr, &m_renderPass.m_renderpass));

		m_renderPass.m_info.clearColor_ = true;
		m_renderPass.m_info.clearDepth_ = true;


		m_vulkanRenderContext.GetResourceManager().AddVulkanRenderpass(m_renderPass.m_renderpass);

	}*/


	
	void IndirectRenderer::UpdateDescriptorSets()
	{
		using namespace ErrorCheck;
		auto lv_totalNumSwapchains = m_vulkanRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();
		auto& lv_vulkanResourceManager = m_vulkanRenderContext.GetResourceManager();

		std::vector<VkDescriptorBufferInfo> lv_bufferInfos{};
		lv_bufferInfos.resize(lv_totalNumSwapchains * 6);

		std::vector<VkDescriptorImageInfo> lv_imageInfos{};
		lv_imageInfos.reserve(256);

		std::vector<VkWriteDescriptorSet> lv_writes{};
		lv_writes.reserve(lv_imageInfos.size() + lv_bufferInfos.size());

		for (uint32_t j = 0; j < (uint32_t)m_textureHandlesOfScene.size(); ++j) {
			auto& lv_sceneTexture = lv_vulkanResourceManager.RetrieveGpuTexture(m_textureHandlesOfScene[j]);

			lv_imageInfos.push_back(VkDescriptorImageInfo{
				.sampler = lv_sceneTexture.sampler,
				.imageView = lv_sceneTexture.image.imageView,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
		}

		auto& lv_vertexBuffer = lv_vulkanResourceManager.RetrieveGpuBuffer(m_vertexDataBufferHandle);
		lv_bufferInfos[0].buffer = lv_vertexBuffer.buffer;
		lv_bufferInfos[0].offset = 0;
		lv_bufferInfos[0].range = m_vertexBufferSize;

		auto& lv_indexBuffer = lv_vulkanResourceManager.RetrieveGpuBuffer(m_indicesDataBufferHandle);
		lv_bufferInfos[1].buffer = lv_indexBuffer.buffer;
		lv_bufferInfos[1].offset = m_vertexBufferSize;
		lv_bufferInfos[1].range = m_indexBufferSize;

		auto& lv_materialBuffer = lv_vulkanResourceManager.RetrieveGpuBuffer(m_materialBufferHandle);
		lv_bufferInfos[2].buffer = lv_materialBuffer.buffer;
		lv_bufferInfos[2].offset = 0;
		lv_bufferInfos[2].range = lv_materialBuffer.size;
		

		for (uint32_t i = 0; i < lv_totalNumSwapchains; ++i) {


			lv_writes.push_back(VkWriteDescriptorSet{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = m_descriptorSets[i],
				.dstBinding = 1,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pImageInfo = nullptr,
				.pBufferInfo = &lv_bufferInfos[0],
				.pTexelBufferView = nullptr });


			

			lv_writes.push_back(VkWriteDescriptorSet{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = m_descriptorSets[i],
				.dstBinding = 2,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pImageInfo = nullptr,
				.pBufferInfo = &lv_bufferInfos[1],
				.pTexelBufferView = nullptr });

			

			lv_writes.push_back(VkWriteDescriptorSet{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = m_descriptorSets[i],
				.dstBinding = 5,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pImageInfo = nullptr,
				.pBufferInfo = &lv_bufferInfos[2],
				.pTexelBufferView = nullptr });


			

			lv_writes.push_back(VkWriteDescriptorSet{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = m_descriptorSets[i],
				.dstBinding = 6,
				.dstArrayElement = 0,
				.descriptorCount = (uint32_t)lv_imageInfos.size(),
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = lv_imageInfos.data(),
				.pBufferInfo = nullptr,
				.pTexelBufferView = nullptr });


			auto& lv_uniformBuffer = (m_uniformBuffers)[i];
			lv_bufferInfos[3 * i + 3].buffer = lv_uniformBuffer.buffer;
			lv_bufferInfos[3 * i + 3].offset = 0;
			lv_bufferInfos[3 * i + 3].range = lv_uniformBuffer.size;


			lv_writes.push_back(VkWriteDescriptorSet{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = nullptr,
			.dstSet = m_descriptorSets[i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pImageInfo = nullptr,
			.pBufferInfo = &lv_bufferInfos[3 * i + 3],
			.pTexelBufferView = nullptr });


			auto& lv_drawDataBuffer = lv_vulkanResourceManager.RetrieveGpuBuffer(m_instanceBuffersGpu[i]);
			lv_bufferInfos[3 * i + 4].buffer = lv_drawDataBuffer.buffer;
			lv_bufferInfos[3 * i + 4].offset = 0;
			lv_bufferInfos[3 * i + 4].range = lv_drawDataBuffer.size;

			lv_writes.push_back(VkWriteDescriptorSet{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = nullptr,
			.dstSet = m_descriptorSets[i],
			.dstBinding = 3,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pImageInfo = nullptr,
			.pBufferInfo = &lv_bufferInfos[3 * i + 4],
			.pTexelBufferView = nullptr });

			auto& lv_transformationBuffer = lv_vulkanResourceManager.RetrieveGpuBuffer(m_transformationsBufferHandles[i]);
			lv_bufferInfos[3 * i + 5].buffer = lv_transformationBuffer.buffer;
			lv_bufferInfos[3 * i + 5].offset = 0;
			lv_bufferInfos[3 * i + 5].range = lv_transformationBuffer.size;

			lv_writes.push_back(VkWriteDescriptorSet{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = nullptr,
			.dstSet = m_descriptorSets[i],
			.dstBinding = 4,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pImageInfo = nullptr,
			.pBufferInfo = &lv_bufferInfos[3 * i + 5],
			.pTexelBufferView = nullptr });

		}
		
		vkUpdateDescriptorSets(m_vulkanRenderContext.GetContextCreator().m_vkDev.m_device,
			lv_writes.size(), lv_writes.data(), 0, nullptr);

	}

	void IndirectRenderer::LoadInstanceData(const char* l_instanceFile)
	{
		FILE* lv_instanceFile = fopen(l_instanceFile, "rb");

		if (nullptr == lv_instanceFile) {
			printf("Failed to open instance file.\n");
			exit(EXIT_FAILURE);
		}


		uint32_t lv_totalNumberInstances{};

		if (1 != fread(&lv_totalNumberInstances, sizeof(uint32_t), 1, lv_instanceFile)) {
			printf("\nFailed to read total number of instances.\n");
			exit(EXIT_FAILURE);
		}

		m_outputInstanceData.resize(lv_totalNumberInstances);
		m_totalNumInstances = m_outputInstanceData.size();
		if (m_totalNumInstances != 
			fread(m_outputInstanceData.data(), sizeof(InstanceData), m_outputInstanceData.size(), lv_instanceFile)) {

			printf("Could not successfully load instance data.\n");
			exit(EXIT_FAILURE);
		}

		fclose(lv_instanceFile);
	}



	uint32_t IndirectRenderer::GetVertexBufferSize()
	{
		return m_vertexBufferSize;
	}


	MeshConverter::MeshFileHeader IndirectRenderer::LoadMeshData(const char* l_meshFileHeader)
	{
		using namespace MeshConverter;

		FILE* lv_meshFileHeader = fopen(l_meshFileHeader, "rb");

		if (nullptr == lv_meshFileHeader) {
			printf("Failed to open instance file.\n");
			exit(EXIT_FAILURE);
		}

		MeshFileHeader lv_meshFileHeaderInstance{};

		if (sizeof(MeshFileHeader) != 
			fread(&lv_meshFileHeaderInstance, 1, sizeof(MeshFileHeader), lv_meshFileHeader)) {

			printf("Failed to read header of mesh file.\n");
			exit(EXIT_FAILURE);
		}

		m_vertexBuffers.resize(lv_meshFileHeaderInstance.m_vertexDataSize / sizeof(float));
		m_indexBuffers.resize(lv_meshFileHeaderInstance.m_lodDataSize/sizeof(unsigned int));
		m_meshes.resize(lv_meshFileHeaderInstance.m_meshCount);

		if (lv_meshFileHeaderInstance.m_meshCount * sizeof(Mesh) !=
			fread(m_meshes.data(), 1, lv_meshFileHeaderInstance.m_meshCount*sizeof(Mesh), lv_meshFileHeader)) {
			printf("Failed to read mesh instance datas from mesh file header.\n");
			exit(EXIT_FAILURE);
		}

		if (lv_meshFileHeaderInstance.m_vertexDataSize !=
			fread(m_vertexBuffers.data(), 1, lv_meshFileHeaderInstance.m_vertexDataSize, lv_meshFileHeader)) {
			printf("Failed to read vertex buffer datas from mesh file header.\n");
			exit(EXIT_FAILURE);
		}

		if (lv_meshFileHeaderInstance.m_lodDataSize !=
			fread(m_indexBuffers.data(), 1, lv_meshFileHeaderInstance.m_lodDataSize, lv_meshFileHeader)) {
			printf("Failed to read mesh instance datas from mesh file header.\n");
			exit(EXIT_FAILURE);
		}


		fclose(lv_meshFileHeader);

		return lv_meshFileHeaderInstance;
	}


	MeshConverter::GeometryConverter::BoundingBox IndirectRenderer::LoadBoundingBoxData(const char* l_boundingBoxFile)
	{
		FILE* lv_boundingBoxFile = fopen(l_boundingBoxFile, "rb");

		if (nullptr == lv_boundingBoxFile) {
			printf("Failed to open bounding box file.\n");
			exit(EXIT_FAILURE);
		}

		uint32_t lv_totalNumBoundingBoxes{};

		if (1 != fread(&lv_totalNumBoundingBoxes, sizeof(uint32_t), 1, lv_boundingBoxFile)) {
			printf("Failed to read total number of bounding boxes from the bounding box file.\n");
			exit(EXIT_FAILURE);
		}

		m_boundingBoxes.resize(lv_totalNumBoundingBoxes);

		if (lv_totalNumBoundingBoxes != fread(m_boundingBoxes.data(), sizeof(MeshConverter::GeometryConverter::BoundingBox), lv_totalNumBoundingBoxes, lv_boundingBoxFile)) {
			printf("Failed to read the data of bounding boxes from the bounding box file.\n");
			exit(EXIT_FAILURE);
		}
	}
}