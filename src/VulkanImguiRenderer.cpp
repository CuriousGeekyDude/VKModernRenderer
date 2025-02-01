


#include "VulkanImguiRenderer.hpp"
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "ErrorCheck.hpp"


namespace RenderCore
{
	using glm::mat4;
	using glm::vec2;
	using glm::vec3;
	using glm::vec4;

	constexpr uint32_t ImGuiVtxBufferSize = 512 * 1024 * sizeof(ImDrawVert);
	constexpr uint32_t ImGuiIdxBufferSize = 512 * 1024 * sizeof(uint32_t);

	bool ImGuiRenderer::createDescriptorSet(VulkanRenderDevice& vkDev)
	{
		using namespace ErrorCheck;

		const std::array<VkDescriptorSetLayoutBinding, 4> bindings = {
			descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
			descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
			descriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
			descriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		const VkDescriptorSetLayoutCreateInfo layoutInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.bindingCount = static_cast<uint32_t>(bindings.size()),
			.pBindings = bindings.data()
		};

		VULKAN_CHECK(vkCreateDescriptorSetLayout(vkDev.m_device, &layoutInfo, nullptr, &m_descriptorSetLayout));

		std::vector<VkDescriptorSetLayout> layouts(vkDev.m_swapchainImages.size(), m_descriptorSetLayout);

		const VkDescriptorSetAllocateInfo allocInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = nullptr,
			.descriptorPool = m_descriptorPool,
			.descriptorSetCount = static_cast<uint32_t>(vkDev.m_swapchainImages.size()),
			.pSetLayouts = layouts.data()
		};

		m_descriptorSet.resize(vkDev.m_swapchainImages.size());

		VULKAN_CHECK(vkAllocateDescriptorSets(vkDev.m_device, &allocInfo, m_descriptorSet.data()));

		for (size_t i = 0; i < vkDev.m_swapchainImages.size(); i++)
		{
			VkDescriptorSet ds = m_descriptorSet[i];
			const VkDescriptorBufferInfo bufferInfo = { m_uniformBuffers[i], 0, sizeof(mat4) };
			const VkDescriptorBufferInfo bufferInfo2 = { storageBuffer_[i], 0, ImGuiVtxBufferSize };
			const VkDescriptorBufferInfo bufferInfo3 = { storageBuffer_[i], ImGuiVtxBufferSize, ImGuiIdxBufferSize };
			const VkDescriptorImageInfo  imageInfo = { fontSampler_, font_.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

			const std::array<VkWriteDescriptorSet, 4> descriptorWrites = {
				bufferWriteDescriptorSet(ds, &bufferInfo,  0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
				bufferWriteDescriptorSet(ds, &bufferInfo2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
				bufferWriteDescriptorSet(ds, &bufferInfo3, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
				imageWriteDescriptorSet(ds, &imageInfo,   3)
			};

			vkUpdateDescriptorSets(vkDev.m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}

		return true;
	}

	bool ImGuiRenderer::createMultiDescriptorSet(VulkanRenderDevice& vkDev)
	{
		const std::array<VkDescriptorSetLayoutBinding, 4> bindings = {
			descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
			descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
			descriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
			descriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, static_cast<uint32_t>(1 + extTextures_.size()))
		};

		const VkDescriptorSetLayoutCreateInfo layoutInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.bindingCount = static_cast<uint32_t>(bindings.size()),
			.pBindings = bindings.data()
		};

		VULKAN_CHECK(vkCreateDescriptorSetLayout(vkDev.m_device, &layoutInfo, nullptr, &m_descriptorSetLayout));

		std::vector<VkDescriptorSetLayout> layouts(vkDev.m_swapchainImages.size(), m_descriptorSetLayout);

		const VkDescriptorSetAllocateInfo allocInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = nullptr,
			.descriptorPool = m_descriptorPool,
			.descriptorSetCount = static_cast<uint32_t>(vkDev.m_swapchainImages.size()),
			.pSetLayouts = layouts.data()
		};

		m_descriptorSet.resize(vkDev.m_swapchainImages.size());

		VULKAN_CHECK(vkAllocateDescriptorSets(vkDev.m_device, &allocInfo, m_descriptorSet.data()));

		// use the font texture initialized in the constructor
		std::vector<VkDescriptorImageInfo> textureDescriptors = {
			{ fontSampler_, font_.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
		};

		for (size_t i = 0; i < extTextures_.size(); i++) {
			textureDescriptors.push_back({
				.sampler = extTextures_[i].sampler,
				.imageView = extTextures_[i].image.imageView,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL   /// TODO: select type from VulkanTexture object (GENERAL or SHADER_READ_ONLY_OPTIMAL)
				});
		}

		for (size_t i = 0; i < vkDev.m_swapchainImages.size(); i++)
		{
			VkDescriptorSet ds = m_descriptorSet[i];
			const VkDescriptorBufferInfo bufferInfo = { m_uniformBuffers[i], 0, sizeof(mat4) };
			const VkDescriptorBufferInfo bufferInfo2 = { storageBuffer_[i], 0, ImGuiVtxBufferSize };
			const VkDescriptorBufferInfo bufferInfo3 = { storageBuffer_[i], ImGuiVtxBufferSize, ImGuiIdxBufferSize };

			const std::array<VkWriteDescriptorSet, 4> descriptorWrites = {
				bufferWriteDescriptorSet(ds, &bufferInfo,  0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
				bufferWriteDescriptorSet(ds, &bufferInfo2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
				bufferWriteDescriptorSet(ds, &bufferInfo3, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
				VkWriteDescriptorSet {
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = m_descriptorSet[i],
					.dstBinding = 3,
					.dstArrayElement = 0,
					.descriptorCount = static_cast<uint32_t>(1 + extTextures_.size()),
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo = textureDescriptors.data()
				},
			};

			vkUpdateDescriptorSets(vkDev.m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}

		return true;
	}

	void addImGuiItem(uint32_t width, uint32_t height, VkCommandBuffer commandBuffer, const ImDrawCmd* pcmd, ImVec2 clipOff, ImVec2 clipScale, int idxOffset, int vtxOffset,
		const std::vector<VulkanTexture>& textures, VkPipelineLayout pipelineLayout)
	{
		if (pcmd->UserCallback)
			return;

		// Project scissor/clipping rectangles into framebuffer space
		ImVec4 clipRect;
		clipRect.x = (pcmd->ClipRect.x - clipOff.x) * clipScale.x;
		clipRect.y = (pcmd->ClipRect.y - clipOff.y) * clipScale.y;
		clipRect.z = (pcmd->ClipRect.z - clipOff.x) * clipScale.x;
		clipRect.w = (pcmd->ClipRect.w - clipOff.y) * clipScale.y;

		if (clipRect.x < width && clipRect.y < height && clipRect.z >= 0.0f && clipRect.w >= 0.0f)
		{
			if (clipRect.x < 0.0f) clipRect.x = 0.0f;
			if (clipRect.y < 0.0f) clipRect.y = 0.0f;
			// Apply scissor/clipping rectangle
			const VkRect2D scissor = {
				.offset = {.x = (int32_t)(clipRect.x), .y = (int32_t)(clipRect.y) },
				.extent = {.width = (uint32_t)(clipRect.z - clipRect.x), .height = (uint32_t)(clipRect.w - clipRect.y) }
			};
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

			// this is added in the Chapter 6: Using descriptor indexing in Vulkan to render an ImGui UI
			if (textures.size() > 0)
			{
				uint32_t texture = (uint32_t)(intptr_t)pcmd->TextureId;
				vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), (const void*)&texture);
			}

			vkCmdDraw(commandBuffer,
				pcmd->ElemCount,
				1,
				pcmd->IdxOffset + idxOffset,
				pcmd->VtxOffset + vtxOffset);
		}
	}

	void ImGuiRenderer::FillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t currentImage, VkFramebuffer l_frameBuffer = VK_NULL_HANDLE,
		VkRenderPass l_renderpass = VK_NULL_HANDLE)
	{
		

		BeginRenderPass(commandBuffer, currentImage);

		ImVec2 clipOff = drawData->DisplayPos;         // (0,0) unless using multi-viewports
		ImVec2 clipScale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

		int vtxOffset = 0;
		int idxOffset = 0;

		for (int n = 0; n < drawData->CmdListsCount; n++)
		{
			const ImDrawList* cmdList = drawData->CmdLists[n];

			for (int cmd = 0; cmd < cmdList->CmdBuffer.Size; cmd++)
			{
				const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmd];

				addImGuiItem(m_frameBufferWidth, m_frameBufferHeight, commandBuffer,
					pcmd, clipOff, clipScale, idxOffset, vtxOffset, extTextures_, m_pipelineLayout);
			}
			idxOffset += cmdList->IdxBuffer.Size;
			vtxOffset += cmdList->VtxBuffer.Size;
		}

		vkCmdEndRenderPass(commandBuffer);
	}

	void ImGuiRenderer::updateBuffers(VulkanRenderDevice& vkDev, uint32_t currentImage, const ImDrawData* imguiDrawData)
	{
		drawData = imguiDrawData;

		const float L = drawData->DisplayPos.x;
		const float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
		const float T = drawData->DisplayPos.y;
		const float B = drawData->DisplayPos.y + drawData->DisplaySize.y;

		const mat4 inMtx = glm::ortho(L, R, T, B);

		memcpy(m_mappedUniformData[currentImage], glm::value_ptr(inMtx), sizeof(mat4));

		void* data = nullptr;
		vkMapMemory(vkDev.m_device, storageBufferMemory_[currentImage], 0, bufferSize_, 0, &data);

		ImDrawVert* vtx = (ImDrawVert*)data;
		for (int n = 0; n < drawData->CmdListsCount; n++)
		{
			const ImDrawList* cmdList = drawData->CmdLists[n];
			memcpy(vtx, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
			vtx += cmdList->VtxBuffer.Size;
		}

		uint32_t* idx = (uint32_t*)((uint8_t*)data + ImGuiVtxBufferSize);
		for (int n = 0; n < drawData->CmdListsCount; n++)
		{
			const ImDrawList* cmdList = drawData->CmdLists[n];
			const uint16_t* src = (const uint16_t*)cmdList->IdxBuffer.Data;

			for (int j = 0; j < cmdList->IdxBuffer.Size; j++)
				*idx++ = (uint32_t)*src++;
		}

		vkUnmapMemory(vkDev.m_device, storageBufferMemory_[currentImage]);
	}

	bool createFontTexture(ImGuiIO& io, const char* fontFile, VulkanRenderDevice& vkDev, 
		VkImage& textureImage, VkDeviceMemory& textureImageMemory)
	{
		// Build texture atlas
		ImFontConfig cfg = ImFontConfig();
		cfg.FontDataOwnedByAtlas = false;
		cfg.RasterizerMultiply = 1.5f;
		cfg.SizePixels = 768.0f / 32.0f;
		cfg.PixelSnapH = true;
		cfg.OversampleH = 4;
		cfg.OversampleV = 4;
		ImFont* Font = io.Fonts->AddFontFromFileTTF(fontFile, cfg.SizePixels, &cfg);

		unsigned char* pixels = nullptr;
		int texWidth = 1, texHeight = 1;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &texWidth, &texHeight);

		if (!pixels || !createTextureImageFromData(vkDev, textureImage, textureImageMemory, pixels, texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM))
		{
			printf("Failed to load texture\n"); fflush(stdout);
			return false;
		}

		io.Fonts->TexID = (ImTextureID)0;
		io.FontDefault = Font;
		io.DisplayFramebufferScale = ImVec2(1, 1);

		return true;
	}

	ImGuiRenderer::ImGuiRenderer(VulkanRenderDevice& vkDev)
		: Renderbase(vkDev, VulkanImage())
	{
		// Resource loading
		ImGuiIO& io = ImGui::GetIO();
		createFontTexture(io, "data/OpenSans-Light.ttf", vkDev, font_.image, font_.imageMemory);

		createImageView(vkDev.m_device, font_.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &font_.imageView);
		createTextureSampler(vkDev.m_device, &fontSampler_);

		// Buffer allocation
		const size_t imgCount = vkDev.m_swapchainImages.size();

		storageBuffer_.resize(imgCount);
		storageBufferMemory_.resize(imgCount);

		bufferSize_ = ImGuiVtxBufferSize + ImGuiIdxBufferSize;

		for (size_t i = 0; i < imgCount; i++)
		{
			if (!createBuffer(vkDev.m_device, vkDev.m_physicalDevice, bufferSize_,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				storageBuffer_[i], storageBufferMemory_[i]))
			{
				printf("ImGuiRenderer: createBuffer() failed\n");
				exit(EXIT_FAILURE);
			}
		}

		// Pipeline creation
		if (!createColorAndDepthRenderPass(vkDev, false, &m_renderPass, RenderPassCreateInfo()) ||
			!createColorAndDepthFramebuffers(vkDev, m_renderPass, VK_NULL_HANDLE, m_frameBuffers) ||
			!CreateUniformBuffers(vkDev, sizeof(mat4)) ||
			!createDescriptorPool(vkDev, 1, 2, 1, &m_descriptorPool) ||
			!createDescriptorSet(vkDev) ||
			!createPipelineLayout(vkDev.m_device, m_descriptorSetLayout, &m_pipelineLayout) ||
			!createGraphicsPipeline(vkDev, m_renderPass, m_pipelineLayout,
				{ "data/shaders/chapter07/VK02_ImGui.vert", "data/shaders/chapter07/VK02_ImGui.frag" },
				&m_graphicsPipeline, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				true, true, true))
		{
			printf("ImGuiRenderer: pipeline creation failed\n");
			exit(EXIT_FAILURE);
		}
	}

	ImGuiRenderer::ImGuiRenderer(VulkanRenderDevice& vkDev, const std::vector<VulkanTexture>& textures)
		: Renderbase(vkDev, VulkanImage()),
		extTextures_(textures)
	{
		// Resource loading
		ImGuiIO& io = ImGui::GetIO();
		createFontTexture(io, "data/OpenSans-Light.ttf", vkDev, font_.image, font_.imageMemory);

		createImageView(vkDev.m_device, font_.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &font_.imageView);
		createTextureSampler(vkDev.m_device, &fontSampler_);

		// Buffer allocation
		const size_t imgCount = vkDev.m_swapchainImages.size();

		storageBuffer_.resize(imgCount);
		storageBufferMemory_.resize(imgCount);

		bufferSize_ = ImGuiVtxBufferSize + ImGuiIdxBufferSize;

		for (size_t i = 0; i < imgCount; i++)
		{
			if (!createBuffer(vkDev.m_device, vkDev.m_physicalDevice, bufferSize_,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				storageBuffer_[i], storageBufferMemory_[i]))
			{
				printf("ImGuiRenderer: createBuffer() failed\n");
				exit(EXIT_FAILURE);
			}
		}

		// Pipeline creation
		if (!createColorAndDepthRenderPass(vkDev, false, &m_renderPass, RenderPassCreateInfo()) ||
			!createColorAndDepthFramebuffers(vkDev, m_renderPass, VK_NULL_HANDLE, m_frameBuffers) ||
			!CreateUniformBuffers(vkDev, sizeof(mat4)) ||
			!createDescriptorPool(vkDev, 1, 2, 1 + static_cast<uint32_t>(textures.size()), &m_descriptorPool) ||
			!createMultiDescriptorSet(vkDev) ||
			!createPipelineLayoutWithConstants(vkDev.m_device, m_descriptorSetLayout, &m_pipelineLayout, 0, sizeof(uint32_t)) ||
			!createGraphicsPipeline(vkDev, m_renderPass, m_pipelineLayout,
				{ "data/shaders/chapter04/imgui.vert", "data/shaders/chapter06/imgui_multi.frag" }, &m_graphicsPipeline, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				true, true, true))
		{
			printf("ImGuiRenderer: pipeline creation failed\n");
			exit(EXIT_FAILURE);
		}
	}

	ImGuiRenderer::~ImGuiRenderer()
	{
		for (size_t i = 0; i < m_frameBuffers.size(); i++)
		{
			vkDestroyBuffer(m_device, storageBuffer_[i], nullptr);
			vkFreeMemory(m_device, storageBufferMemory_[i], nullptr);
		}

		vkDestroySampler(m_device, fontSampler_, nullptr);
		destroyVulkanImage(m_device, font_);
	}
}