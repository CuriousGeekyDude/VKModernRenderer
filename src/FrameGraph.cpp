





#include "FrameGraph.hpp"
#include "VulkanRenderContext.hpp"
#include <fstream>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <iostream>
#include <stack>
#include <optional>
#include "ErrorCheck.hpp"
#include <format>
#include <utility>
#include "Renderbase.hpp"
#include "CameraStructure.hpp"
#include <algorithm>

namespace VulkanEngine
{
	FrameGraph::FrameGraph(const std::string& l_jsonFilePath,
		VulkanRenderContext& l_vkRenderContext) :
		m_vkRenderContext(l_vkRenderContext)
	{

        m_totalNumNodesPerCmdBuffer.resize(m_vkRenderContext.GetContextCreator().m_vkDev.m_totalNumCmdBufferLeft2[0]);


        std::ifstream lv_graphJSONFile(l_jsonFilePath);
        rapidjson::IStreamWrapper lv_isw(lv_graphJSONFile);

        rapidjson::Document lv_document;
        const rapidjson::ParseResult lv_parseResult = lv_document.ParseStream(lv_isw);

        if (lv_parseResult.IsError() == true) {
            std::cout << lv_parseResult.Code() << std::endl;
            exit(-1);
        }



        if (lv_document.HasMember("FrameGraphName") == true) {
            m_frameGraphName = lv_document["FrameGraphName"].GetString();
        }
        else {
            std::cout << "Frame graph json file has no name! Exitting....\n" << std::endl;
            exit(-1);
        }



        if (lv_document.HasMember("RenderPasses") == true) {
            
            m_nodes.resize(lv_document["RenderPasses"].Size());
            m_nodeHandles.reserve(lv_document["RenderPasses"].Size());
            


            //We need to parse the name of the nodes first 
            // in order to create the edges later in one go along with the input and outputs
            for (size_t i = 0; i < lv_document["RenderPasses"].Size(); ++i) {
                FrameGraphNode& lv_node = m_nodes[i];
                lv_node.m_nodeNames = lv_document["RenderPasses"][i]["Name"].GetString();
                lv_node.m_nodeIndex = (uint32_t)i;
            }


            size_t lv_resourceIndex = 0;
            for (size_t i = 0; i < lv_document["RenderPasses"].Size(); ++i) {
                
                FrameGraphNode& lv_node = m_nodes[i];

                if (lv_document["RenderPasses"][i].HasMember("Name") == true) {

                    auto& lv_renderPass = lv_document["RenderPasses"][i];

                    m_nodeHandles.push_back(i);
                    lv_node.m_outputResourcesHandles.resize(lv_renderPass["Output"].Size());
                    lv_node.m_inputResourcesHandles.resize(lv_renderPass["Input"].Size());
                    lv_node.m_pipelineType = lv_renderPass["Pipeline"].GetString();
                    lv_node.m_targetNodesHandles.resize(lv_renderPass["TargetNodes"].Size());

                    if (0 == strcmp(lv_renderPass["RenderToCubemap"].GetString(), "FALSE")) {
                        lv_node.m_renderToCubemap = false;
                    }
                    else {
                        lv_node.m_renderToCubemap = true;
                    }

                    lv_node.m_cubemapFace = lv_renderPass["CubemapFace"].GetInt();

                    for (size_t j = 0; j < lv_renderPass["Output"].Size(); ++j, ++lv_resourceIndex) {

                        lv_node.m_outputResourcesHandles[j] = lv_resourceIndex;

                        FrameGraphResource lv_outputResource;
                        FrameGraphResourceInfo lv_outputInfo;
                        auto& lv_outputResources = lv_renderPass["Output"][j];

                        lv_outputResource.m_resourceName = lv_outputResources["Name"].GetString();
                        
                        const char* lv_createOnGPUString = lv_outputResources["CreateResourceOnGPU"].GetString();

                        if (strcmp(lv_createOnGPUString, "FALSE") == 0) {
                            lv_outputInfo.m_createOnGPU = false;
                        }
                        else {
                            lv_outputInfo.m_createOnGPU = true;
                        }

                        lv_outputResource.m_nodeThatOwnsThisResourceHandle = i;

                        lv_outputInfo.m_storeOp = StringToStoreOp(lv_outputResources["TextureInfo"][0]["StoreOp"].GetString());
                        lv_outputInfo.m_imageLayout = StringToVkImageLayout(lv_outputResources["TextureInfo"][0]["ImageLayout"].GetString());


                        if (lv_outputInfo.m_createOnGPU == true) {
                            lv_outputInfo.m_depth = lv_outputResources["TextureInfo"][0]["Resolution"][2].GetUint();
                            lv_outputInfo.m_height = lv_outputResources["TextureInfo"][0]["Resolution"][1].GetUint();
                            lv_outputInfo.m_width = lv_outputResources["TextureInfo"][0]["Resolution"][0].GetUint();
                            lv_outputInfo.m_format = StringToVkFormat(lv_outputResources["TextureInfo"][0]["Format"].GetString());
                        }
                        

                        lv_outputResource.m_Info = lv_outputInfo;

                        m_frameGraphResources.push_back(lv_outputResource);
                        m_frameGraphResourcesHandles.push_back(lv_resourceIndex);
                    }


                    for (size_t j = 0; j < lv_renderPass["Input"].Size(); ++j, ++lv_resourceIndex) {

                        lv_node.m_inputResourcesHandles[j] = lv_resourceIndex;

                        FrameGraphResource lv_inputResource;
                        FrameGraphResourceInfo lv_inputInfo;
                        auto& lv_inputResources = lv_renderPass["Input"][j];

                        lv_inputResource.m_resourceName = lv_inputResources["Name"].GetString();

                        const char* lv_createOnGPUString = lv_inputResources["CreateResourceOnGPU"].GetString();

                        if (strcmp(lv_createOnGPUString, "FALSE") == 0) {
                            lv_inputInfo.m_createOnGPU = false;
                        }
                        else {
                            lv_inputInfo.m_createOnGPU = true;
                        }


                        if (true == lv_inputResources["TextureInfo"][0].HasMember("MipLevel")) {
                            lv_inputInfo.m_mipLevels = (uint32_t)lv_inputResources["TextureInfo"][0]["MipLevel"].GetInt();
                        }

                        if (true == lv_inputResources["TextureInfo"][0].HasMember("SamplerMode")) {
                            lv_inputInfo.m_addressMode = StringToVkSamplerAddressMode(lv_inputResources["TextureInfo"][0]["SamplerMode"].GetString());
                        }


                        lv_inputResource.m_nodeThatOwnsThisResourceHandle = i;

                        lv_inputInfo.m_loadOp = StringToLoadOp(lv_inputResources["TextureInfo"][0]["LoadOp"].GetString());
                        lv_inputInfo.m_imageLayout = StringToVkImageLayout(lv_inputResources["TextureInfo"][0]["ImageLayout"].GetString());


                        if (lv_inputInfo.m_createOnGPU == true) {
                            lv_inputInfo.m_depth = lv_inputResources["TextureInfo"][0]["Resolution"][2].GetUint();
                            lv_inputInfo.m_height = lv_inputResources["TextureInfo"][0]["Resolution"][1].GetUint();
                            lv_inputInfo.m_width = lv_inputResources["TextureInfo"][0]["Resolution"][0].GetUint();
                            lv_inputInfo.m_format = StringToVkFormat(lv_inputResources["TextureInfo"][0]["Format"].GetString());
                        }


                        lv_inputResource.m_Info = lv_inputInfo;

                        m_frameGraphResources.push_back(lv_inputResource);
                        m_frameGraphResourcesHandles.push_back(lv_resourceIndex);
                    }


                    for (size_t j = 0; j < lv_renderPass["TargetNodes"].Size(); ++j) {

                        auto& lv_targetNodeObjectJSON = lv_renderPass["TargetNodes"][j];

                        for (auto& l_node : m_nodes) {
                            if (strcmp(lv_targetNodeObjectJSON["Name"].GetString(), l_node.m_nodeNames.c_str()) == 0) {
                                lv_node.m_targetNodesHandles.push_back(l_node.m_nodeIndex);
                                break;
                            }
                        }

                    }
 
                }
                else {
                    std::cout << "One of the nodes of the frame graph lacks a name. Exitting...." << std::endl;
                    exit(-1);
                }

            }


            std::stack<uint32_t> lv_stackOfNodes;
            std::vector<uint8_t> lv_visitedNodes;
            std::vector<uint32_t> lv_sortedNodeHandles;

            lv_visitedNodes.resize(m_nodes.size());

            //Sort the frame graph nodes
            for (auto l_nodeHandle : m_nodeHandles) {

                lv_stackOfNodes.push(l_nodeHandle);

                while (lv_stackOfNodes.size() > 0) {

                    auto lv_nodeHandle = lv_stackOfNodes.top();

                    if (lv_visitedNodes[lv_nodeHandle] == 2) {
                        lv_stackOfNodes.pop();
                        continue;
                    }

                    if (lv_visitedNodes[lv_nodeHandle] == 1) {
                        lv_visitedNodes[lv_nodeHandle] = 2;
                        lv_sortedNodeHandles.push_back(lv_nodeHandle);

                        lv_stackOfNodes.pop();
                        continue;
                    }

                    lv_visitedNodes[lv_nodeHandle] = 1;

                    auto& lv_node = m_nodes[lv_nodeHandle];

                    for (auto l_edgeNodeHandle : lv_node.m_targetNodesHandles) {

                        if (lv_visitedNodes[l_edgeNodeHandle] == 0) {
                            lv_stackOfNodes.push(l_edgeNodeHandle);
                        }
                    }


                }
            }

            for (size_t i = 0; i < lv_sortedNodeHandles.size(); ++i) {
                m_nodeHandles[i] = lv_sortedNodeHandles[lv_sortedNodeHandles.size() - i - 1];
            }

            for (auto l_nodeHandle : m_nodeHandles) {
                CreateRenderpassAndFramebuffers(l_nodeHandle);
            }
        }
        else {
            std::cout << "There are no render passes in the frame graph json file. Exitting...." << std::endl;
            exit(-1);
        }

	}




    FrameGraphNode* FrameGraph::RetrieveNode(const std::string& l_nodeName)
    {
        for (auto& l_node : m_nodes) {
            if (l_nodeName == l_node.m_nodeNames) {
                return &l_node;
            }
        }

        return nullptr;
    }


    void FrameGraph::CreateRenderpassAndFramebuffers(const uint32_t l_nodeHandle)
    {

        using namespace ErrorCheck;
        auto& lv_vkResManager = m_vkRenderContext.GetResourceManager();
        auto lv_totalNumSwapchains = m_vkRenderContext.GetContextCreator().m_vkDev.m_swapchainImages.size();

        auto& lv_node = m_nodes[l_nodeHandle];

        if (lv_node.m_pipelineType == "GRAPHIC") {

            if (lv_node.m_inputResourcesHandles.empty() == false) {

                std::optional<uint32_t> lv_depthResourceHandle;
                for (size_t i = 0; i < lv_node.m_inputResourcesHandles.size(); ++i) {
                    auto& lv_inputResource = m_frameGraphResources[lv_node.m_inputResourcesHandles[i]];

                    if (lv_inputResource.m_resourceName.substr(0, 5) == "Depth") {
                        lv_depthResourceHandle.emplace(i);
                        break;
                    }
                }


                //swap the depth handle and the last element of the m_inputResourcesHandles vector
                //in order to ensure the VkAttachmentDescription of the depth attachment comes last
                //in the std::vector<VkAttachmentDescription> lv_attachmentDescriptions 
                if (true == lv_depthResourceHandle.has_value() && lv_depthResourceHandle.value() != lv_node.m_inputResourcesHandles.back()) {
                    std::swap(lv_node.m_inputResourcesHandles[lv_depthResourceHandle.value()],
                        lv_node.m_inputResourcesHandles[lv_node.m_inputResourcesHandles.size() - 1]);
                }

                std::vector<VkAttachmentDescription> lv_attachmentDescriptions;
                std::vector<VkAttachmentReference> lv_attachmentReferences;
                std::vector<const char*> lv_attachmentNames;
                std::vector<uint8_t> lv_attachmentBits;
                std::vector<uint32_t> lv_framebufferTexturesHandles;

                for (auto l_inputResourceHandle : lv_node.m_inputResourcesHandles) {

                    auto& lv_inputResource = m_frameGraphResources[l_inputResourceHandle];
                    FrameGraphResource lv_correspondingOutputResource;
                    for (auto l_outputResourceHandle : lv_node.m_outputResourcesHandles) {

                        auto& lv_outputResource = m_frameGraphResources[l_outputResourceHandle];

                        if (strcmp(lv_outputResource.m_resourceName.c_str(), lv_inputResource.m_resourceName.c_str()) == 0) {
                            lv_correspondingOutputResource = lv_outputResource;
                            break;
                        }
                    }
                    bool lv_depthTestName = (lv_inputResource.m_resourceName == "Depth");
                    bool lv_swapchainTestName = (lv_inputResource.m_resourceName == "Swapchain");
                    bool lv_depthMapPointLightTestName = (lv_inputResource.m_resourceName == "DepthMapPointLight");
                    VulkanTexture lv_depth, lv_swapchain, lv_depthMapPointLight;

                    if (lv_depthTestName == true) {
                        auto lv_depthMeta = lv_vkResManager.RetrieveGpuResourceMetaData("Depth 0");
                        lv_depth = lv_vkResManager.RetrieveGpuTexture(lv_depthMeta.m_resourceHandle);
                    }
                    if (lv_swapchainTestName == true) {
                        auto lv_swapchainMeta = lv_vkResManager.RetrieveGpuResourceMetaData("Swapchain 0");
                        lv_swapchain = lv_vkResManager.RetrieveGpuTexture(lv_swapchainMeta.m_resourceHandle);
                    }
                    if (lv_depthMapPointLightTestName == true) {
                        auto lv_depthMapPointMeta = lv_vkResManager.RetrieveGpuResourceMetaData("DepthMapPointLight");
                        lv_depthMapPointLight = lv_vkResManager.RetrieveGpuTexture(lv_depthMapPointMeta.m_resourceHandle);

                        lv_inputResource.m_Info.m_format = lv_depthMapPointLight.format;
                        lv_inputResource.m_Info.m_depth = lv_depthMapPointLight.depth;
                        lv_inputResource.m_Info.m_height = lv_depthMapPointLight.height;
                        lv_inputResource.m_Info.m_width = lv_depthMapPointLight.width;
                        lv_inputResource.m_Info.m_imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                    }

                    else if (false == lv_inputResource.m_Info.m_createOnGPU) {

                        VulkanTexture* lv_inputRes;
                        auto lv_inputResMeta = lv_vkResManager.RetrieveGpuResourceMetaData(lv_inputResource.m_resourceName);


                        if (std::numeric_limits<uint32_t>::max() == lv_inputResMeta.m_resourceHandle) {
                            lv_inputRes = &lv_vkResManager.RetrieveGpuTexture(lv_inputResource.m_resourceName, 0);
                        }
                        else {
                            lv_inputRes = &lv_vkResManager.RetrieveGpuTexture(lv_inputResMeta.m_resourceHandle);
                        }

                        lv_inputResource.m_Info.m_format = lv_inputRes->format;
                    }

                    //0 signifies that there is no need to create a new resource
                    //1 signifies that there is a need to create a new resource
                    if (lv_inputResource.m_Info.m_createOnGPU == false) {
                        lv_attachmentBits.push_back(0);
                    }
                    else {
                        lv_attachmentBits.push_back(1);
                    }

                    lv_attachmentNames.push_back(lv_inputResource.m_resourceName.c_str());

                    VkAttachmentDescription lv_attachmentDescription{};
                    lv_attachmentDescription.format = lv_depthTestName ?  lv_depth.format :  lv_swapchainTestName ? lv_swapchain.format :lv_inputResource.m_Info.m_format;
                    lv_attachmentDescription.flags = 0;
                    lv_attachmentDescription.loadOp = lv_inputResource.m_Info.m_loadOp;
                    lv_attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
                    lv_attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                    lv_attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                    lv_attachmentDescription.storeOp = lv_correspondingOutputResource.m_Info.m_storeOp;
                    lv_attachmentDescription.initialLayout = lv_inputResource.m_Info.m_imageLayout;
                    lv_attachmentDescription.finalLayout = lv_correspondingOutputResource.m_Info.m_imageLayout;

                    lv_attachmentDescriptions.push_back(lv_attachmentDescription);

                }


                lv_attachmentReferences.resize(lv_attachmentDescriptions.size());

                for (size_t i = 0; i < lv_attachmentReferences.size(); ++i) {
                    lv_attachmentReferences[i].attachment = i;
                    lv_attachmentReferences[i].layout = lv_attachmentDescriptions[i].initialLayout;
                }

                VkSubpassDescription lv_subpassDescription{};
                lv_subpassDescription.colorAttachmentCount = lv_depthResourceHandle.has_value() == true ?
                    (uint32_t)lv_attachmentReferences.size() - 1 : (uint32_t)lv_attachmentReferences.size();
                lv_subpassDescription.pColorAttachments = lv_attachmentReferences.data();
                lv_subpassDescription.flags = 0;
                lv_subpassDescription.inputAttachmentCount = 0;
                lv_subpassDescription.pDepthStencilAttachment = lv_depthResourceHandle.has_value() == true ? &lv_attachmentReferences[lv_attachmentReferences.size() - 1] : nullptr;
                lv_subpassDescription.pInputAttachments = nullptr;
                lv_subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                lv_subpassDescription.pPreserveAttachments = nullptr;
                lv_subpassDescription.preserveAttachmentCount = 0;
                lv_subpassDescription.pResolveAttachments = nullptr;

                VkRenderPassCreateInfo lv_renderpassCreateInfo{};
                lv_renderpassCreateInfo.attachmentCount = (uint32_t)lv_attachmentDescriptions.size();
                lv_renderpassCreateInfo.dependencyCount = 0;
                lv_renderpassCreateInfo.flags = 0;
                lv_renderpassCreateInfo.pAttachments = lv_attachmentDescriptions.data();
                lv_renderpassCreateInfo.pDependencies = nullptr;
                lv_renderpassCreateInfo.pNext = nullptr;
                lv_renderpassCreateInfo.pSubpasses = &lv_subpassDescription;
                lv_renderpassCreateInfo.subpassCount = 1;
                lv_renderpassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;


                VULKAN_CHECK(vkCreateRenderPass(m_vkRenderContext.GetContextCreator().m_vkDev.m_device, &lv_renderpassCreateInfo,
                    nullptr, &lv_node.m_renderpass));

                lv_vkResManager.AddVulkanRenderpass(lv_node.m_renderpass);

                RenderCore::VulkanResourceManager::RenderPass lv_renderpass;
                lv_renderpass.m_renderpass = lv_node.m_renderpass;
                lv_renderpass.m_info.clearDepth_ = lv_depthResourceHandle.has_value() == false ? false
                    : (lv_attachmentDescriptions.back().loadOp & VK_ATTACHMENT_LOAD_OP_CLEAR) != 0 ? true : false;
                lv_renderpass.m_info.flags_ = 0;
                lv_renderpass.m_info.clearColor_ = (lv_attachmentDescriptions[0].loadOp & VK_ATTACHMENT_LOAD_OP_CLEAR) != 0 ? true : false;

                //It is assumed that we only render to a face of the cubemap each time
                //by issuing a draw call.
                if (true == lv_node.m_renderToCubemap) {

                    lv_node.m_frameBufferHandles.reserve(lv_totalNumSwapchains);

                    auto lv_cubemapMeta = lv_vkResManager.RetrieveGpuResourceMetaData(lv_attachmentNames[0]);

                    assert(lv_cubemapMeta.m_resourceHandle != std::numeric_limits<uint32_t>::max());
                    
                    std::string lv_formattedString{};
                    for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {

                        lv_formattedString = lv_attachmentNames[0] + std::to_string(i) +" {}";
                        auto lv_formattedArgs = std::make_format_args(lv_node.m_cubemapFace);

                        lv_node.m_frameBufferHandles.push_back(lv_vkResManager.CreateFrameBufferCubemapFace(lv_renderpass, lv_cubemapMeta.m_resourceHandle, lv_node.m_cubemapFace, std::vformat(lv_formattedString, lv_formattedArgs).c_str()));

                        
                    }

                }

                //Yes, it was better to have the else content in if block to avoid pipeline stalling
                //, but it doesnt matter since there arent that many renderpasses for this project to begin with.
                else {

                    lv_framebufferTexturesHandles.reserve(lv_totalNumSwapchains * lv_attachmentNames.size());

                    for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {
                        for (size_t j = 0; j < lv_attachmentNames.size(); ++j) {

                            std::string lv_formattedString{ lv_attachmentNames[j] };

                            for (auto l_inputResHandle : lv_node.m_inputResourcesHandles) {

                                auto& lv_inputRes = m_frameGraphResources[l_inputResHandle];

                                if (lv_inputRes.m_resourceName != lv_formattedString) { continue; }


                                lv_formattedString = lv_formattedString + " {}";
                                auto lv_formattedArgs = std::make_format_args(i);

                                if (lv_attachmentBits[j] == 0) {
                                    auto lv_textureMetaData = lv_vkResManager.RetrieveGpuResourceMetaData(std::vformat(lv_formattedString, lv_formattedArgs).c_str());
                                    if (UINT32_MAX == lv_textureMetaData.m_resourceHandle) {
                                        std::cout << "Texture meta data has invalid handle. Exitting...." << std::endl;
                                        exit(-1);
                                    }
                                    lv_framebufferTexturesHandles.push_back(lv_textureMetaData.m_resourceHandle);
                                }
                                else {
                                    if (std::string{ lv_attachmentNames[j] }.substr(0, 5) != "Depth") {
                                        lv_framebufferTexturesHandles.push_back(lv_vkResManager.CreateTexture(m_vkRenderContext.GetContextCreator().m_vkDev.m_maxAnisotropy, std::vformat(lv_formattedString, lv_formattedArgs).c_str(),
                                            lv_attachmentDescriptions[j].format, 704, 704,lv_inputRes.m_Info.m_mipLevels, VK_FILTER_LINEAR, VK_FILTER_LINEAR, lv_inputRes.m_Info.m_addressMode));
                                        lv_vkResManager.AddGpuResource(std::vformat(lv_formattedString, lv_formattedArgs).c_str(), lv_framebufferTexturesHandles.back(), RenderCore::VulkanResourceManager::VulkanDataType::m_texture);
                                    }
                                    else {
                                        lv_framebufferTexturesHandles.push_back(lv_vkResManager.CreateDepthTextureWithHandle(std::vformat(lv_formattedString, lv_formattedArgs).c_str()));
                                        lv_vkResManager.AddGpuResource(std::vformat(lv_formattedString, lv_formattedArgs).c_str(), lv_framebufferTexturesHandles.back(), RenderCore::VulkanResourceManager::VulkanDataType::m_texture);
                                    }
                                }
                            }
                        }
                    }

                    lv_node.m_frameBufferHandles.resize(lv_totalNumSwapchains);

                    for (size_t i = 0; i < lv_totalNumSwapchains; ++i) {

                        std::vector<uint32_t> lv_textureHandles{};
                        lv_textureHandles.reserve(lv_attachmentNames.size());
                        for (size_t j = i * lv_attachmentNames.size(); j < (i + 1) * lv_attachmentNames.size(); ++j) {
                            lv_textureHandles.push_back(lv_framebufferTexturesHandles[j]);
                        }

                        std::string lv_formattedString{ lv_node.m_nodeNames + "Framebuffer {}" };
                        auto lv_formattedArgs = std::make_format_args(i);
                        lv_node.m_frameBufferHandles[i] = lv_vkResManager.CreateFrameBuffer(lv_renderpass, lv_textureHandles, std::vformat(lv_formattedString, lv_formattedArgs).c_str());

                    }

                }

            }
           
                


            }

        
    }


    VkAttachmentStoreOp FrameGraph::StringToStoreOp(const char* l_op)
    {
        if (strcmp(l_op, "VK_ATTACHMENT_STORE_OP_STORE") == 0) {
            return VK_ATTACHMENT_STORE_OP_STORE;
        }
        else if (strcmp(l_op, "VK_ATTACHMENT_STORE_OP_DONT_CARE") == 0) {
            return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }

        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
    VkImageLayout FrameGraph::StringToVkImageLayout(const char* l_op)
    {
        if (strcmp(l_op, "VK_IMAGE_LAYOUT_UNDEFINED") == 0) {
            return VK_IMAGE_LAYOUT_UNDEFINED;
        }
        else if (strcmp(l_op, "VK_IMAGE_LAYOUT_GENERAL") == 0) {
            return VK_IMAGE_LAYOUT_GENERAL;

        }
        else if (strcmp(l_op, "VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL") == 0) {
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        }
        else if (strcmp(l_op, "VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL") == 0) {
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        }
        else if (strcmp(l_op, "VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL") == 0) {
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        }
        else if (strcmp(l_op, "VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL") == 0) {
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        }
        else if (strcmp(l_op, "VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL") == 0) {
            return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        }
        else if (strcmp(l_op, "VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL") == 0) {
            return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        }
        else if (strcmp(l_op, "VK_IMAGE_LAYOUT_PRESENT_SRC_KHR") == 0) {
            return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }
        return VK_IMAGE_LAYOUT_UNDEFINED;

    }



    VkSamplerAddressMode FrameGraph::StringToVkSamplerAddressMode(const char* l_samplerMode)
    {
        if (strcmp(l_samplerMode, "VK_SAMPLER_ADDRESS_MODE_REPEAT") == true) {
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }
        else if (strcmp(l_samplerMode, "VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT") == true) {
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        }
        else if (strcmp(l_samplerMode, "VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE") == true) {
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        }
        else if (strcmp(l_samplerMode, "VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER") == true) {
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        }

        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    }



    void FrameGraphNode::UpdateBuffers(const uint32_t l_currentSwapchainIndex,
        const VulkanEngine::CameraStructure& l_cameraStructure)
    {
        m_renderer->UpdateBuffers(l_currentSwapchainIndex, l_cameraStructure);
    }

    void FrameGraphNode::FillCommandBuffer(VkCommandBuffer l_cmdBuffer,
        uint32_t l_currentSwapchainIndex)
    {
        m_renderer->FillCommandBuffer(l_cmdBuffer, l_currentSwapchainIndex);
    }


    void FrameGraph::RenderGraph(VkCommandBuffer l_cmdBuffer,
        uint32_t l_currentSwapchainIndex)
    {
       
        size_t lv_countNodeIndex{ 0 }, lv_totalNumCmdBuffers{ 0 };
        for (auto lv_totalNumNodes : m_totalNumNodesPerCmdBuffer) {
            if (0 == lv_totalNumNodes) { break; }

            const VkCommandBufferBeginInfo bi =
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext = nullptr,
                .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
                .pInheritanceInfo = nullptr
            };
            assert(lv_totalNumCmdBuffers < m_vkRenderContext.GetContextCreator().m_vkDev.m_mainCommandBuffers2.size());
            VkCommandBuffer commandBuffer = m_vkRenderContext.GetContextCreator().m_vkDev.m_mainCommandBuffers2[lv_totalNumCmdBuffers];


            VK_CHECK(vkBeginCommandBuffer(commandBuffer, &bi));
            for (size_t i = lv_countNodeIndex; i < lv_countNodeIndex + lv_totalNumNodes; ++i) {

                auto lv_nodeHandle = m_nodeHandles[i];
                auto& lv_node = m_nodes[lv_nodeHandle];

                if (true == lv_node.m_enabled) {
                    lv_node.FillCommandBuffer(commandBuffer, l_currentSwapchainIndex);
                }
            }
            VK_CHECK(vkEndCommandBuffer(commandBuffer));

            lv_countNodeIndex += lv_totalNumNodes;
            ++lv_totalNumCmdBuffers;
        }

        const VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; // or even VERTEX_SHADER_STAGE

        const VkSubmitInfo si =
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &m_vkRenderContext.GetContextCreator().m_vkDev.m_binarySemaphore,
            .pWaitDstStageMask = waitStages,
            .commandBufferCount = (uint32_t)lv_totalNumCmdBuffers,
            .pCommandBuffers = m_vkRenderContext.GetContextCreator().m_vkDev.m_mainCommandBuffers2.data(),
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &m_vkRenderContext.GetContextCreator().m_vkDev.m_binarySemaphore
        };

        VK_CHECK(vkQueueSubmit(m_vkRenderContext.GetContextCreator().m_vkDev.m_mainQueue1, 1, &si, nullptr));

        const VkPresentInfoKHR pi =
        {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = nullptr,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &m_vkRenderContext.GetContextCreator().m_vkDev.m_binarySemaphore,
            .swapchainCount = 1,
            .pSwapchains = &m_vkRenderContext.GetContextCreator().m_vkDev.m_swapchain,
            .pImageIndices = &l_currentSwapchainIndex
        };

        VK_CHECK(vkQueuePresentKHR(m_vkRenderContext.GetContextCreator().m_vkDev.m_mainQueue1, &pi));
        VK_CHECK(vkDeviceWaitIdle(m_vkRenderContext.GetContextCreator().m_vkDev.m_device));
    }


    void FrameGraph::UpdateNodes(const uint32_t l_currentSwapchainIndex,
        const VulkanEngine::CameraStructure& l_cameraStructure)
    {
        for (auto l_nodeHandle : m_nodeHandles) {
            auto& lv_node = m_nodes[l_nodeHandle];

            if (true == lv_node.m_enabled) {
                lv_node.UpdateBuffers(l_currentSwapchainIndex, l_cameraStructure);
            }
        }
    }

    void FrameGraph::Debug()
    {
        for (auto l_nodeHandle : m_nodeHandles) {

            auto& lv_node = m_nodes[l_nodeHandle];

            std::cout << "\n\n\n------------- NEW NODE: " << lv_node.m_nodeNames << " --------------" << std::endl;

            for (auto l_inputResourceHandle : lv_node.m_inputResourcesHandles) {

                auto& lv_inputResource = m_frameGraphResources[l_inputResourceHandle];

                std::cout << "\n\nInput resource: " << std::endl;
                std::cout << "Name: " << lv_inputResource.m_resourceName << std::endl;

            }

            for (auto l_outputResourceHandle : lv_node.m_outputResourcesHandles) {
                
                auto& lv_outputResource = m_frameGraphResources[l_outputResourceHandle];

                std::cout << "\n\nOutput resource: " << std::endl;
                std::cout << "Name: " << lv_outputResource.m_resourceName << std::endl;
            }

        }
    }



    uint32_t FrameGraph::FindSortedHandleFromGivenNodeName(const std::string& l_nodeName)
    {
        for (uint32_t i = 0; i < m_nodeHandles.size(); ++i) {
            if (m_nodes[m_nodeHandles[i]].m_nodeNames == l_nodeName) {
                return i;
            }
        }

        return std::numeric_limits<uint32_t>::max();
    }

    void FrameGraph::DisableNodesAfterGivenNodeHandleUntilLast2(const uint32_t l_nodeHandle)
    {
        assert(l_nodeHandle < (uint32_t)(m_nodes.size()));

        for (uint32_t i = l_nodeHandle+1; i < (m_nodeHandles.size()-2); ++i) {
            m_nodes[m_nodeHandles[i]].m_enabled = false;
        }

    }

    void FrameGraph::DisableNodesAfterGivenNodeHandleUntilLast(const uint32_t l_nodeHandle)
    {
        assert(l_nodeHandle < (uint32_t)(m_nodes.size()));

        for (uint32_t i = l_nodeHandle + 1; i < (m_nodeHandles.size() - 2); ++i) {
            m_nodes[m_nodeHandles[i]].m_enabled = false;
        }
    }

    void FrameGraph::EnableAllNodes()
    {
        for (auto& l_node : m_nodes) {
            l_node.m_enabled = true;
        }
    }


    void FrameGraph::IncrementNumNodesPerCmdBuffer(uint32_t l_cmdBufferIndex)
    {
        assert(m_totalNumNodesPerCmdBuffer.size() > l_cmdBufferIndex);

        ++m_totalNumNodesPerCmdBuffer[l_cmdBufferIndex];
    }


    VkFormat FrameGraph::StringToVkFormat(const char* format) {

        if (strcmp(format, "VK_FORMAT_R4G4_UNORM_PACK8") == 0) {
            return VK_FORMAT_R4G4_UNORM_PACK8;
        }
        if (strcmp(format, "VK_FORMAT_R4G4B4A4_UNORM_PACK16") == 0) {
            return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
        }
        if (strcmp(format, "VK_FORMAT_B4G4R4A4_UNORM_PACK16") == 0) {
            return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
        }
        if (strcmp(format, "VK_FORMAT_R5G6B5_UNORM_PACK16") == 0) {
            return VK_FORMAT_R5G6B5_UNORM_PACK16;
        }
        if (strcmp(format, "VK_FORMAT_B5G6R5_UNORM_PACK16") == 0) {
            return VK_FORMAT_B5G6R5_UNORM_PACK16;
        }
        if (strcmp(format, "VK_FORMAT_R5G5B5A1_UNORM_PACK16") == 0) {
            return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
        }
        if (strcmp(format, "VK_FORMAT_B5G5R5A1_UNORM_PACK16") == 0) {
            return VK_FORMAT_B5G5R5A1_UNORM_PACK16;
        }
        if (strcmp(format, "VK_FORMAT_A1R5G5B5_UNORM_PACK16") == 0) {
            return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
        }
        if (strcmp(format, "VK_FORMAT_R8_UNORM") == 0) {
            return VK_FORMAT_R8_UNORM;
        }
        if (strcmp(format, "VK_FORMAT_R8_SNORM") == 0) {
            return VK_FORMAT_R8_SNORM;
        }
        if (strcmp(format, "VK_FORMAT_R8_USCALED") == 0) {
            return VK_FORMAT_R8_USCALED;
        }
        if (strcmp(format, "VK_FORMAT_R8_SSCALED") == 0) {
            return VK_FORMAT_R8_SSCALED;
        }
        if (strcmp(format, "VK_FORMAT_R8_UINT") == 0) {
            return VK_FORMAT_R8_UINT;
        }
        if (strcmp(format, "VK_FORMAT_R8_SINT") == 0) {
            return VK_FORMAT_R8_SINT;
        }
        if (strcmp(format, "VK_FORMAT_R8_SRGB") == 0) {
            return VK_FORMAT_R8_SRGB;
        }
        if (strcmp(format, "VK_FORMAT_R8G8_UNORM") == 0) {
            return VK_FORMAT_R8G8_UNORM;
        }
        if (strcmp(format, "VK_FORMAT_R8G8_SNORM") == 0) {
            return VK_FORMAT_R8G8_SNORM;
        }
        if (strcmp(format, "VK_FORMAT_R8G8_USCALED") == 0) {
            return VK_FORMAT_R8G8_USCALED;
        }
        if (strcmp(format, "VK_FORMAT_R8G8_SSCALED") == 0) {
            return VK_FORMAT_R8G8_SSCALED;
        }
        if (strcmp(format, "VK_FORMAT_R8G8_UINT") == 0) {
            return VK_FORMAT_R8G8_UINT;
        }
        if (strcmp(format, "VK_FORMAT_R8G8_SINT") == 0) {
            return VK_FORMAT_R8G8_SINT;
        }
        if (strcmp(format, "VK_FORMAT_R8G8_SRGB") == 0) {
            return VK_FORMAT_R8G8_SRGB;
        }
        if (strcmp(format, "VK_FORMAT_R8G8B8_UNORM") == 0) {
            return VK_FORMAT_R8G8B8_UNORM;
        }
        if (strcmp(format, "VK_FORMAT_R8G8B8_SNORM") == 0) {
            return VK_FORMAT_R8G8B8_SNORM;
        }
        if (strcmp(format, "VK_FORMAT_R8G8B8_USCALED") == 0) {
            return VK_FORMAT_R8G8B8_USCALED;
        }
        if (strcmp(format, "VK_FORMAT_R8G8B8_SSCALED") == 0) {
            return VK_FORMAT_R8G8B8_SSCALED;
        }
        if (strcmp(format, "VK_FORMAT_R8G8B8_UINT") == 0) {
            return VK_FORMAT_R8G8B8_UINT;
        }
        if (strcmp(format, "VK_FORMAT_R8G8B8_SINT") == 0) {
            return VK_FORMAT_R8G8B8_SINT;
        }
        if (strcmp(format, "VK_FORMAT_R8G8B8_SRGB") == 0) {
            return VK_FORMAT_R8G8B8_SRGB;
        }
        if (strcmp(format, "VK_FORMAT_B8G8R8_UNORM") == 0) {
            return VK_FORMAT_B8G8R8_UNORM;
        }
        if (strcmp(format, "VK_FORMAT_B8G8R8_SNORM") == 0) {
            return VK_FORMAT_B8G8R8_SNORM;
        }
        if (strcmp(format, "VK_FORMAT_B8G8R8_USCALED") == 0) {
            return VK_FORMAT_B8G8R8_USCALED;
        }
        if (strcmp(format, "VK_FORMAT_B8G8R8_SSCALED") == 0) {
            return VK_FORMAT_B8G8R8_SSCALED;
        }
        if (strcmp(format, "VK_FORMAT_B8G8R8_UINT") == 0) {
            return VK_FORMAT_B8G8R8_UINT;
        }
        if (strcmp(format, "VK_FORMAT_B8G8R8_SINT") == 0) {
            return VK_FORMAT_B8G8R8_SINT;
        }
        if (strcmp(format, "VK_FORMAT_B8G8R8_SRGB") == 0) {
            return VK_FORMAT_B8G8R8_SRGB;
        }
        if (strcmp(format, "VK_FORMAT_R8G8B8A8_UNORM") == 0) {
            return VK_FORMAT_R8G8B8A8_UNORM;
        }
        if (strcmp(format, "VK_FORMAT_R8G8B8A8_SNORM") == 0) {
            return VK_FORMAT_R8G8B8A8_SNORM;
        }
        if (strcmp(format, "VK_FORMAT_R8G8B8A8_USCALED") == 0) {
            return VK_FORMAT_R8G8B8A8_USCALED;
        }
        if (strcmp(format, "VK_FORMAT_R8G8B8A8_SSCALED") == 0) {
            return VK_FORMAT_R8G8B8A8_SSCALED;
        }
        if (strcmp(format, "VK_FORMAT_R8G8B8A8_UINT") == 0) {
            return VK_FORMAT_R8G8B8A8_UINT;
        }
        if (strcmp(format, "VK_FORMAT_R8G8B8A8_SINT") == 0) {
            return VK_FORMAT_R8G8B8A8_SINT;
        }
        if (strcmp(format, "VK_FORMAT_R8G8B8A8_SRGB") == 0) {
            return VK_FORMAT_R8G8B8A8_SRGB;
        }
        if (strcmp(format, "VK_FORMAT_B8G8R8A8_UNORM") == 0) {
            return VK_FORMAT_B8G8R8A8_UNORM;
        }
        if (strcmp(format, "VK_FORMAT_B8G8R8A8_SNORM") == 0) {
            return VK_FORMAT_B8G8R8A8_SNORM;
        }
        if (strcmp(format, "VK_FORMAT_B8G8R8A8_USCALED") == 0) {
            return VK_FORMAT_B8G8R8A8_USCALED;
        }
        if (strcmp(format, "VK_FORMAT_B8G8R8A8_SSCALED") == 0) {
            return VK_FORMAT_B8G8R8A8_SSCALED;
        }
        if (strcmp(format, "VK_FORMAT_B8G8R8A8_UINT") == 0) {
            return VK_FORMAT_B8G8R8A8_UINT;
        }
        if (strcmp(format, "VK_FORMAT_B8G8R8A8_SINT") == 0) {
            return VK_FORMAT_B8G8R8A8_SINT;
        }
        if (strcmp(format, "VK_FORMAT_B8G8R8A8_SRGB") == 0) {
            return VK_FORMAT_B8G8R8A8_SRGB;
        }
        if (strcmp(format, "VK_FORMAT_A8B8G8R8_UNORM_PACK32") == 0) {
            return VK_FORMAT_A8B8G8R8_UNORM_PACK32;
        }
        if (strcmp(format, "VK_FORMAT_A8B8G8R8_SNORM_PACK32") == 0) {
            return VK_FORMAT_A8B8G8R8_SNORM_PACK32;
        }
        if (strcmp(format, "VK_FORMAT_A8B8G8R8_USCALED_PACK32") == 0) {
            return VK_FORMAT_A8B8G8R8_USCALED_PACK32;
        }
        if (strcmp(format, "VK_FORMAT_A8B8G8R8_SSCALED_PACK32") == 0) {
            return VK_FORMAT_A8B8G8R8_SSCALED_PACK32;
        }
        if (strcmp(format, "VK_FORMAT_A8B8G8R8_UINT_PACK32") == 0) {
            return VK_FORMAT_A8B8G8R8_UINT_PACK32;
        }
        if (strcmp(format, "VK_FORMAT_A8B8G8R8_SINT_PACK32") == 0) {
            return VK_FORMAT_A8B8G8R8_SINT_PACK32;
        }
        if (strcmp(format, "VK_FORMAT_A8B8G8R8_SRGB_PACK32") == 0) {
            return VK_FORMAT_A8B8G8R8_SRGB_PACK32;
        }
        if (strcmp(format, "VK_FORMAT_A2R10G10B10_UNORM_PACK32") == 0) {
            return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
        }
        if (strcmp(format, "VK_FORMAT_A2R10G10B10_SNORM_PACK32") == 0) {
            return VK_FORMAT_A2R10G10B10_SNORM_PACK32;
        }
        if (strcmp(format, "VK_FORMAT_A2R10G10B10_USCALED_PACK32") == 0) {
            return VK_FORMAT_A2R10G10B10_USCALED_PACK32;
        }
        if (strcmp(format, "VK_FORMAT_A2R10G10B10_SSCALED_PACK32") == 0) {
            return VK_FORMAT_A2R10G10B10_SSCALED_PACK32;
        }
        if (strcmp(format, "VK_FORMAT_A2R10G10B10_UINT_PACK32") == 0) {
            return VK_FORMAT_A2R10G10B10_UINT_PACK32;
        }
        if (strcmp(format, "VK_FORMAT_A2R10G10B10_SINT_PACK32") == 0) {
            return VK_FORMAT_A2R10G10B10_SINT_PACK32;
        }
        if (strcmp(format, "VK_FORMAT_A2B10G10R10_UNORM_PACK32") == 0) {
            return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
        }
        if (strcmp(format, "VK_FORMAT_A2B10G10R10_SNORM_PACK32") == 0) {
            return VK_FORMAT_A2B10G10R10_SNORM_PACK32;
        }
        if (strcmp(format, "VK_FORMAT_A2B10G10R10_USCALED_PACK32") == 0) {
            return VK_FORMAT_A2B10G10R10_USCALED_PACK32;
        }
        if (strcmp(format, "VK_FORMAT_A2B10G10R10_SSCALED_PACK32") == 0) {
            return VK_FORMAT_A2B10G10R10_SSCALED_PACK32;
        }
        if (strcmp(format, "VK_FORMAT_A2B10G10R10_UINT_PACK32") == 0) {
            return VK_FORMAT_A2B10G10R10_UINT_PACK32;
        }
        if (strcmp(format, "VK_FORMAT_A2B10G10R10_SINT_PACK32") == 0) {
            return VK_FORMAT_A2B10G10R10_SINT_PACK32;
        }
        if (strcmp(format, "VK_FORMAT_R16_UNORM") == 0) {
            return VK_FORMAT_R16_UNORM;
        }
        if (strcmp(format, "VK_FORMAT_R16_SNORM") == 0) {
            return VK_FORMAT_R16_SNORM;
        }
        if (strcmp(format, "VK_FORMAT_R16_USCALED") == 0) {
            return VK_FORMAT_R16_USCALED;
        }
        if (strcmp(format, "VK_FORMAT_R16_SSCALED") == 0) {
            return VK_FORMAT_R16_SSCALED;
        }
        if (strcmp(format, "VK_FORMAT_R16_UINT") == 0) {
            return VK_FORMAT_R16_UINT;
        }
        if (strcmp(format, "VK_FORMAT_R16_SINT") == 0) {
            return VK_FORMAT_R16_SINT;
        }
        if (strcmp(format, "VK_FORMAT_R16_SFLOAT") == 0) {
            return VK_FORMAT_R16_SFLOAT;
        }
        if (strcmp(format, "VK_FORMAT_R16G16_UNORM") == 0) {
            return VK_FORMAT_R16G16_UNORM;
        }
        if (strcmp(format, "VK_FORMAT_R16G16_SNORM") == 0) {
            return VK_FORMAT_R16G16_SNORM;
        }
        if (strcmp(format, "VK_FORMAT_R16G16_USCALED") == 0) {
            return VK_FORMAT_R16G16_USCALED;
        }
        if (strcmp(format, "VK_FORMAT_R16G16_SSCALED") == 0) {
            return VK_FORMAT_R16G16_SSCALED;
        }
        if (strcmp(format, "VK_FORMAT_R16G16_UINT") == 0) {
            return VK_FORMAT_R16G16_UINT;
        }
        if (strcmp(format, "VK_FORMAT_R16G16_SINT") == 0) {
            return VK_FORMAT_R16G16_SINT;
        }
        if (strcmp(format, "VK_FORMAT_R16G16_SFLOAT") == 0) {
            return VK_FORMAT_R16G16_SFLOAT;
        }
        if (strcmp(format, "VK_FORMAT_R16G16B16_UNORM") == 0) {
            return VK_FORMAT_R16G16B16_UNORM;
        }
        if (strcmp(format, "VK_FORMAT_R16G16B16_SNORM") == 0) {
            return VK_FORMAT_R16G16B16_SNORM;
        }
        if (strcmp(format, "VK_FORMAT_R16G16B16_USCALED") == 0) {
            return VK_FORMAT_R16G16B16_USCALED;
        }
        if (strcmp(format, "VK_FORMAT_R16G16B16_SSCALED") == 0) {
            return VK_FORMAT_R16G16B16_SSCALED;
        }
        if (strcmp(format, "VK_FORMAT_R16G16B16_UINT") == 0) {
            return VK_FORMAT_R16G16B16_UINT;
        }
        if (strcmp(format, "VK_FORMAT_R16G16B16_SINT") == 0) {
            return VK_FORMAT_R16G16B16_SINT;
        }
        if (strcmp(format, "VK_FORMAT_R16G16B16_SFLOAT") == 0) {
            return VK_FORMAT_R16G16B16_SFLOAT;
        }
        if (strcmp(format, "VK_FORMAT_R16G16B16A16_UNORM") == 0) {
            return VK_FORMAT_R16G16B16A16_UNORM;
        }
        if (strcmp(format, "VK_FORMAT_R16G16B16A16_SNORM") == 0) {
            return VK_FORMAT_R16G16B16A16_SNORM;
        }
        if (strcmp(format, "VK_FORMAT_R16G16B16A16_USCALED") == 0) {
            return VK_FORMAT_R16G16B16A16_USCALED;
        }
        if (strcmp(format, "VK_FORMAT_R16G16B16A16_SSCALED") == 0) {
            return VK_FORMAT_R16G16B16A16_SSCALED;
        }
        if (strcmp(format, "VK_FORMAT_R16G16B16A16_UINT") == 0) {
            return VK_FORMAT_R16G16B16A16_UINT;
        }
        if (strcmp(format, "VK_FORMAT_R16G16B16A16_SINT") == 0) {
            return VK_FORMAT_R16G16B16A16_SINT;
        }
        if (strcmp(format, "VK_FORMAT_R16G16B16A16_SFLOAT") == 0) {
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        }
        if (strcmp(format, "VK_FORMAT_R32_UINT") == 0) {
            return VK_FORMAT_R32_UINT;
        }
        if (strcmp(format, "VK_FORMAT_R32_SINT") == 0) {
            return VK_FORMAT_R32_SINT;
        }
        if (strcmp(format, "VK_FORMAT_R32_SFLOAT") == 0) {
            return VK_FORMAT_R32_SFLOAT;
        }
        if (strcmp(format, "VK_FORMAT_R32G32_UINT") == 0) {
            return VK_FORMAT_R32G32_UINT;
        }
        if (strcmp(format, "VK_FORMAT_R32G32_SINT") == 0) {
            return VK_FORMAT_R32G32_SINT;
        }
        if (strcmp(format, "VK_FORMAT_R32G32_SFLOAT") == 0) {
            return VK_FORMAT_R32G32_SFLOAT;
        }
        if (strcmp(format, "VK_FORMAT_R32G32B32_UINT") == 0) {
            return VK_FORMAT_R32G32B32_UINT;
        }
        if (strcmp(format, "VK_FORMAT_R32G32B32_SINT") == 0) {
            return VK_FORMAT_R32G32B32_SINT;
        }
        if (strcmp(format, "VK_FORMAT_R32G32B32_SFLOAT") == 0) {
            return VK_FORMAT_R32G32B32_SFLOAT;
        }
        if (strcmp(format, "VK_FORMAT_R32G32B32A32_UINT") == 0) {
            return VK_FORMAT_R32G32B32A32_UINT;
        }
        if (strcmp(format, "VK_FORMAT_R32G32B32A32_SINT") == 0) {
            return VK_FORMAT_R32G32B32A32_SINT;
        }
        if (strcmp(format, "VK_FORMAT_R32G32B32A32_SFLOAT") == 0) {
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        }
        if (strcmp(format, "VK_FORMAT_R64_UINT") == 0) {
            return VK_FORMAT_R64_UINT;
        }
        if (strcmp(format, "VK_FORMAT_R64_SINT") == 0) {
            return VK_FORMAT_R64_SINT;
        }
        if (strcmp(format, "VK_FORMAT_R64_SFLOAT") == 0) {
            return VK_FORMAT_R64_SFLOAT;
        }
        if (strcmp(format, "VK_FORMAT_R64G64_UINT") == 0) {
            return VK_FORMAT_R64G64_UINT;
        }
        if (strcmp(format, "VK_FORMAT_R64G64_SINT") == 0) {
            return VK_FORMAT_R64G64_SINT;
        }
        if (strcmp(format, "VK_FORMAT_R64G64_SFLOAT") == 0) {
            return VK_FORMAT_R64G64_SFLOAT;
        }
        if (strcmp(format, "VK_FORMAT_R64G64B64_UINT") == 0) {
            return VK_FORMAT_R64G64B64_UINT;
        }
        if (strcmp(format, "VK_FORMAT_R64G64B64_SINT") == 0) {
            return VK_FORMAT_R64G64B64_SINT;
        }
        if (strcmp(format, "VK_FORMAT_R64G64B64_SFLOAT") == 0) {
            return VK_FORMAT_R64G64B64_SFLOAT;
        }
        if (strcmp(format, "VK_FORMAT_R64G64B64A64_UINT") == 0) {
            return VK_FORMAT_R64G64B64A64_UINT;
        }
        if (strcmp(format, "VK_FORMAT_R64G64B64A64_SINT") == 0) {
            return VK_FORMAT_R64G64B64A64_SINT;
        }
        if (strcmp(format, "VK_FORMAT_R64G64B64A64_SFLOAT") == 0) {
            return VK_FORMAT_R64G64B64A64_SFLOAT;
        }
        if (strcmp(format, "VK_FORMAT_B10G11R11_UFLOAT_PACK32") == 0) {
            return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
        }
        if (strcmp(format, "VK_FORMAT_E5B9G9R9_UFLOAT_PACK32") == 0) {
            return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
        }
        if (strcmp(format, "VK_FORMAT_D16_UNORM") == 0) {
            return VK_FORMAT_D16_UNORM;
        }
        if (strcmp(format, "VK_FORMAT_X8_D24_UNORM_PACK32") == 0) {
            return VK_FORMAT_X8_D24_UNORM_PACK32;
        }
        if (strcmp(format, "VK_FORMAT_D32_SFLOAT") == 0) {
            return VK_FORMAT_D32_SFLOAT;
        }
        if (strcmp(format, "VK_FORMAT_S8_UINT") == 0) {
            return VK_FORMAT_S8_UINT;
        }
        if (strcmp(format, "VK_FORMAT_D16_UNORM_S8_UINT") == 0) {
            return VK_FORMAT_D16_UNORM_S8_UINT;
        }
        if (strcmp(format, "VK_FORMAT_D24_UNORM_S8_UINT") == 0) {
            return VK_FORMAT_D24_UNORM_S8_UINT;
        }
        if (strcmp(format, "VK_FORMAT_D32_SFLOAT_S8_UINT") == 0) {
            return VK_FORMAT_D32_SFLOAT_S8_UINT;
        }
        if (strcmp(format, "VK_FORMAT_BC1_RGB_UNORM_BLOCK") == 0) {
            return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_BC1_RGB_SRGB_BLOCK") == 0) {
            return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_BC1_RGBA_UNORM_BLOCK") == 0) {
            return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_BC1_RGBA_SRGB_BLOCK") == 0) {
            return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_BC2_UNORM_BLOCK") == 0) {
            return VK_FORMAT_BC2_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_BC2_SRGB_BLOCK") == 0) {
            return VK_FORMAT_BC2_SRGB_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_BC3_UNORM_BLOCK") == 0) {
            return VK_FORMAT_BC3_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_BC3_SRGB_BLOCK") == 0) {
            return VK_FORMAT_BC3_SRGB_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_BC4_UNORM_BLOCK") == 0) {
            return VK_FORMAT_BC4_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_BC4_SNORM_BLOCK") == 0) {
            return VK_FORMAT_BC4_SNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_BC5_UNORM_BLOCK") == 0) {
            return VK_FORMAT_BC5_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_BC5_SNORM_BLOCK") == 0) {
            return VK_FORMAT_BC5_SNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_BC6H_UFLOAT_BLOCK") == 0) {
            return VK_FORMAT_BC6H_UFLOAT_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_BC6H_SFLOAT_BLOCK") == 0) {
            return VK_FORMAT_BC6H_SFLOAT_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_BC7_UNORM_BLOCK") == 0) {
            return VK_FORMAT_BC7_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_BC7_SRGB_BLOCK") == 0) {
            return VK_FORMAT_BC7_SRGB_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK") == 0) {
            return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK") == 0) {
            return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK") == 0) {
            return VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK") == 0) {
            return VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK") == 0) {
            return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK") == 0) {
            return VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_EAC_R11_UNORM_BLOCK") == 0) {
            return VK_FORMAT_EAC_R11_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_EAC_R11_SNORM_BLOCK") == 0) {
            return VK_FORMAT_EAC_R11_SNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_EAC_R11G11_UNORM_BLOCK") == 0) {
            return VK_FORMAT_EAC_R11G11_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_EAC_R11G11_SNORM_BLOCK") == 0) {
            return VK_FORMAT_EAC_R11G11_SNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_4x4_UNORM_BLOCK") == 0) {
            return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_4x4_SRGB_BLOCK") == 0) {
            return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_5x4_UNORM_BLOCK") == 0) {
            return VK_FORMAT_ASTC_5x4_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_5x4_SRGB_BLOCK") == 0) {
            return VK_FORMAT_ASTC_5x4_SRGB_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_5x5_UNORM_BLOCK") == 0) {
            return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_5x5_SRGB_BLOCK") == 0) {
            return VK_FORMAT_ASTC_5x5_SRGB_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_6x5_UNORM_BLOCK") == 0) {
            return VK_FORMAT_ASTC_6x5_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_6x5_SRGB_BLOCK") == 0) {
            return VK_FORMAT_ASTC_6x5_SRGB_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_6x6_UNORM_BLOCK") == 0) {
            return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_6x6_SRGB_BLOCK") == 0) {
            return VK_FORMAT_ASTC_6x6_SRGB_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_8x5_UNORM_BLOCK") == 0) {
            return VK_FORMAT_ASTC_8x5_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_8x5_SRGB_BLOCK") == 0) {
            return VK_FORMAT_ASTC_8x5_SRGB_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_8x6_UNORM_BLOCK") == 0) {
            return VK_FORMAT_ASTC_8x6_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_8x6_SRGB_BLOCK") == 0) {
            return VK_FORMAT_ASTC_8x6_SRGB_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_8x8_UNORM_BLOCK") == 0) {
            return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_8x8_SRGB_BLOCK") == 0) {
            return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_10x5_UNORM_BLOCK") == 0) {
            return VK_FORMAT_ASTC_10x5_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_10x5_SRGB_BLOCK") == 0) {
            return VK_FORMAT_ASTC_10x5_SRGB_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_10x6_UNORM_BLOCK") == 0) {
            return VK_FORMAT_ASTC_10x6_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_10x6_SRGB_BLOCK") == 0) {
            return VK_FORMAT_ASTC_10x6_SRGB_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_10x8_UNORM_BLOCK") == 0) {
            return VK_FORMAT_ASTC_10x8_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_10x8_SRGB_BLOCK") == 0) {
            return VK_FORMAT_ASTC_10x8_SRGB_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_10x10_UNORM_BLOCK") == 0) {
            return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_10x10_SRGB_BLOCK") == 0) {
            return VK_FORMAT_ASTC_10x10_SRGB_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_12x10_UNORM_BLOCK") == 0) {
            return VK_FORMAT_ASTC_12x10_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_12x10_SRGB_BLOCK") == 0) {
            return VK_FORMAT_ASTC_12x10_SRGB_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_12x12_UNORM_BLOCK") == 0) {
            return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_12x12_SRGB_BLOCK") == 0) {
            return VK_FORMAT_ASTC_12x12_SRGB_BLOCK;
        }
        if (strcmp(format, "VK_FORMAT_G8B8G8R8_422_UNORM") == 0) {
            return VK_FORMAT_G8B8G8R8_422_UNORM;
        }
        if (strcmp(format, "VK_FORMAT_B8G8R8G8_422_UNORM") == 0) {
            return VK_FORMAT_B8G8R8G8_422_UNORM;
        }
        if (strcmp(format, "VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM") == 0) {
            return VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM;
        }
        if (strcmp(format, "VK_FORMAT_G8_B8R8_2PLANE_420_UNORM") == 0) {
            return VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
        }
        if (strcmp(format, "VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM") == 0) {
            return VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM;
        }
        if (strcmp(format, "VK_FORMAT_G8_B8R8_2PLANE_422_UNORM") == 0) {
            return VK_FORMAT_G8_B8R8_2PLANE_422_UNORM;
        }
        if (strcmp(format, "VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM") == 0) {
            return VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM;
        }
        if (strcmp(format, "VK_FORMAT_R10X6_UNORM_PACK16") == 0) {
            return VK_FORMAT_R10X6_UNORM_PACK16;
        }
        if (strcmp(format, "VK_FORMAT_R10X6G10X6_UNORM_2PACK16") == 0) {
            return VK_FORMAT_R10X6G10X6_UNORM_2PACK16;
        }
        if (strcmp(format, "VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16") == 0) {
            return VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16;
        }
        if (strcmp(format, "VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16") == 0) {
            return VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16;
        }
        if (strcmp(format, "VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16") == 0) {
            return VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16;
        }
        if (strcmp(format, "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16") == 0) {
            return VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16;
        }
        if (strcmp(format, "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16") == 0) {
            return VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16;
        }
        if (strcmp(format, "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16") == 0) {
            return VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16;
        }
        if (strcmp(format, "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16") == 0) {
            return VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16;
        }
        if (strcmp(format, "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16") == 0) {
            return VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16;
        }
        if (strcmp(format, "VK_FORMAT_R12X4_UNORM_PACK16") == 0) {
            return VK_FORMAT_R12X4_UNORM_PACK16;
        }
        if (strcmp(format, "VK_FORMAT_R12X4G12X4_UNORM_2PACK16") == 0) {
            return VK_FORMAT_R12X4G12X4_UNORM_2PACK16;
        }
        if (strcmp(format, "VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16") == 0) {
            return VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16;
        }
        if (strcmp(format, "VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16") == 0) {
            return VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16;
        }
        if (strcmp(format, "VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16") == 0) {
            return VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16;
        }
        if (strcmp(format, "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16") == 0) {
            return VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16;
        }
        if (strcmp(format, "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16") == 0) {
            return VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16;
        }
        if (strcmp(format, "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16") == 0) {
            return VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16;
        }
        if (strcmp(format, "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16") == 0) {
            return VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16;
        }
        if (strcmp(format, "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16") == 0) {
            return VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16;
        }
        if (strcmp(format, "VK_FORMAT_G16B16G16R16_422_UNORM") == 0) {
            return VK_FORMAT_G16B16G16R16_422_UNORM;
        }
        if (strcmp(format, "VK_FORMAT_B16G16R16G16_422_UNORM") == 0) {
            return VK_FORMAT_B16G16R16G16_422_UNORM;
        }
        if (strcmp(format, "VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM") == 0) {
            return VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM;
        }
        if (strcmp(format, "VK_FORMAT_G16_B16R16_2PLANE_420_UNORM") == 0) {
            return VK_FORMAT_G16_B16R16_2PLANE_420_UNORM;
        }
        if (strcmp(format, "VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM") == 0) {
            return VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM;
        }
        if (strcmp(format, "VK_FORMAT_G16_B16R16_2PLANE_422_UNORM") == 0) {
            return VK_FORMAT_G16_B16R16_2PLANE_422_UNORM;
        }
        if (strcmp(format, "VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM") == 0) {
            return VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM;
        }
        if (strcmp(format, "VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG") == 0) {
            return VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG;
        }
        if (strcmp(format, "VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG") == 0) {
            return VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG;
        }
        if (strcmp(format, "VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG") == 0) {
            return VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG;
        }
        if (strcmp(format, "VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG") == 0) {
            return VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG;
        }
        if (strcmp(format, "VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG") == 0) {
            return VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG;
        }
        if (strcmp(format, "VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG") == 0) {
            return VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG;
        }
        if (strcmp(format, "VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG") == 0) {
            return VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG;
        }
        if (strcmp(format, "VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG") == 0) {
            return VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT") == 0) {
            return VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT") == 0) {
            return VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT") == 0) {
            return VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT") == 0) {
            return VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT") == 0) {
            return VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT") == 0) {
            return VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT") == 0) {
            return VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT") == 0) {
            return VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT") == 0) {
            return VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT") == 0) {
            return VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT") == 0) {
            return VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT") == 0) {
            return VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT") == 0) {
            return VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT;
        }
        if (strcmp(format, "VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT") == 0) {
            return VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT;
        }
        if (strcmp(format, "VK_FORMAT_G8_B8R8_2PLANE_444_UNORM_EXT") == 0) {
            return VK_FORMAT_G8_B8R8_2PLANE_444_UNORM_EXT;
        }
        if (strcmp(format, "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16_EXT") == 0) {
            return VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16_EXT;
        }
        if (strcmp(format, "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16_EXT") == 0) {
            return VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16_EXT;
        }
        if (strcmp(format, "VK_FORMAT_G16_B16R16_2PLANE_444_UNORM_EXT") == 0) {
            return VK_FORMAT_G16_B16R16_2PLANE_444_UNORM_EXT;
        }
        if (strcmp(format, "VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT") == 0) {
            return VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT;
        }
        if (strcmp(format, "VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT") == 0) {
            return VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT;
        }

        return VK_FORMAT_UNDEFINED;
    }

    VkAttachmentLoadOp FrameGraph::StringToLoadOp(const char* l_op)
    {
        if (strcmp(l_op, "VK_ATTACHMENT_LOAD_OP_LOAD") == 0) {
            return VK_ATTACHMENT_LOAD_OP_LOAD;
        }
        if (strcmp(l_op, "VK_ATTACHMENT_LOAD_OP_CLEAR") == 0) {
            return VK_ATTACHMENT_LOAD_OP_CLEAR;
        }
        if (strcmp(l_op, "VK_ATTACHMENT_LOAD_OP_DONT_CARE") == 0) {
            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        }
        

        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }

}