#pragma once


#include <volk.h>
#include "spirv_reflect.h"

#include <vector>
#include <string>

namespace VulkanEngine
{
	class SpirvPipelineGenerator
	{
	public:

		struct VulkanDescriptorSetLayoutData
		{
			std::vector<VkDescriptorSetLayoutBinding> m_bindings;
			std::vector<VkDescriptorBindingFlags> m_flags;
			uint32_t m_setNumber;
			VkDescriptorSetLayoutCreateInfo m_setLayoutCreateInfo;
			VkDescriptorSetLayoutBindingFlagsCreateInfo m_setLayoutBindingFlagsCreateInfo;
		};


	public:

		SpirvPipelineGenerator(const std::string& l_spirvFilePath);


		const std::vector<VulkanDescriptorSetLayoutData>& 
			GenerateDescriptorSetLayouts();



		void DebugPipelineGenerator();


		~SpirvPipelineGenerator();
	private:

		SpvReflectShaderModule m_spvModule;
		uint32_t m_totalNumSets;
		std::vector<SpvReflectDescriptorSet*> m_spvDescriptorSets;
		std::vector<uint8_t> m_spirvBlob;
		std::vector<VulkanDescriptorSetLayoutData> m_setLayoutDatas;



	};
}