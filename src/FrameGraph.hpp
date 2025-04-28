#pragma once



#include <cinttypes>
#include <string>
#include <vector>
#include <unordered_map>
#include "volk.h"



namespace RenderCore
{
	class Renderbase;
}


namespace VulkanEngine
{

	class VulkanRenderContext;
	struct CameraStructure;



	struct FrameGraphResourceInfo
	{
		bool m_createOnGPU = false;

		uint32_t	m_width = 0;
        uint32_t	m_height = 0;
        uint32_t	m_depth = 0;
		uint32_t	m_mipLevels = 1;

		VkSamplerAddressMode m_addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

        VkFormat	m_format = VK_FORMAT_UNDEFINED;
        VkImageUsageFlags	m_flags = 0;

		VkImageLayout m_imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VkAttachmentLoadOp	m_loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		VkAttachmentStoreOp m_storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		uint32_t	m_textureHandle = UINT32_MAX;
	};



	struct FrameGraphResource
	{
		std::string m_resourceName;
		FrameGraphResourceInfo m_Info;
		uint32_t m_nodeThatOwnsThisResourceHandle;
	};


	struct FrameGraphNode
	{
		void UpdateBuffers(const uint32_t l_currentSwapchainIndex,
			const VulkanEngine::CameraStructure& l_cameraStructure);

		void FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex);

		VkRenderPass m_renderpass = VK_NULL_HANDLE;
		std::vector<uint32_t> m_frameBufferHandles;

		RenderCore::Renderbase* m_renderer = nullptr;

		std::vector<uint32_t> m_inputResourcesHandles;
		std::vector<uint32_t> m_outputResourcesHandles;
		std::vector<uint32_t> m_targetNodesHandles;

		std::string m_nodeNames;
		std::string m_pipelineType;
		uint32_t m_nodeIndex;
		int m_cubemapFace{ -1 };
		bool m_enabled{ true };
		bool m_renderToCubemap{ true };
	};





	class FrameGraph
	{
	
	public:


		FrameGraph(const std::string& l_jsonFilePath,
			VulkanRenderContext& l_vkRenderContext);

		void Debug();

		void IncrementNumNodesPerCmdBuffer(uint32_t l_cmdBufferIndex);

		FrameGraphNode* RetrieveNode(const std::string& l_nodeName);


		void DisableNodesAfterGivenNodeHandleUntilLast2(const uint32_t l_nodeHandle);


		void DisableNodesAfterGivenNodeHandleUntilLast(const uint32_t l_nodeHandle);

		void UpdateNodes(const uint32_t l_currentSwapchainIndex,
			const VulkanEngine::CameraStructure& l_cameraStructure);

		void RenderGraph(VkCommandBuffer l_cmdBuffer,
			uint32_t l_currentSwapchainIndex);


		uint32_t FindSortedHandleFromGivenNodeName(const std::string& l_nodeName);

		void EnableAllNodes();

	protected:

		VkFormat StringToVkFormat(const char* format);
		VkAttachmentLoadOp StringToLoadOp(const char* l_op);

		void CreateRenderpassAndFramebuffers(const uint32_t l_nodeHandle);

		VkAttachmentStoreOp StringToStoreOp(const char* l_op);
		VkImageLayout StringToVkImageLayout(const char* l_op);
		VkSamplerAddressMode StringToVkSamplerAddressMode(const char* l_samplerMode);

	private:

		VulkanRenderContext& m_vkRenderContext;
		std::vector<FrameGraphNode> m_nodes;
		std::vector<FrameGraphResource> m_frameGraphResources;
		std::vector<uint32_t> m_frameGraphResourcesHandles;
		std::vector<uint32_t> m_nodeHandles;
		std::vector<uint32_t> m_totalNumNodesPerCmdBuffer;
		std::string m_frameGraphName;

	};


}