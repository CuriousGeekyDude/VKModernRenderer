


#include "SpirvPipelineGenerator.hpp"
#include <iostream>
#include <cassert>

namespace VulkanEngine
{

	SpirvPipelineGenerator::SpirvPipelineGenerator(const std::string& l_spirvFilePath)
	{
		FILE* lv_spirvFile = fopen(l_spirvFilePath.c_str(), "rb");

		if (nullptr == lv_spirvFile) {
			std::cout << "Failed to open the file: " << l_spirvFilePath << ".\n" << std::endl;
			exit(-1);
		}

		auto lv_result = fseek(lv_spirvFile, 0, SEEK_END);

		if (0 != lv_result) {
			std::cout << "fseek() failed for: " << l_spirvFilePath << ".\n" << std::endl;
			exit(-1);
		}

		uint32_t lv_spirvFileSize = ftell(lv_spirvFile);
		fseek(lv_spirvFile, 0, SEEK_SET);


		m_spirvBlob.resize(lv_spirvFileSize);

		auto lv_totalNumRead = fread(m_spirvBlob.data(), 1, lv_spirvFileSize, lv_spirvFile);

		assert(lv_totalNumRead == lv_spirvFileSize);

		fclose(lv_spirvFile);


		auto lv_spirvResult = spvReflectCreateShaderModule(m_spirvBlob.size(), m_spirvBlob.data(), &m_spvModule);
		assert(SPV_REFLECT_RESULT_SUCCESS == lv_spirvResult);


		lv_spirvResult = spvReflectEnumerateDescriptorSets(&m_spvModule, &m_totalNumSets, nullptr);
		assert(SPV_REFLECT_RESULT_SUCCESS == lv_spirvResult);

		m_spvDescriptorSets.resize(m_totalNumSets);
		m_setLayoutDatas.resize(m_totalNumSets);
		lv_spirvResult = spvReflectEnumerateDescriptorSets(&m_spvModule, &m_totalNumSets, m_spvDescriptorSets.data());
		assert(SPV_REFLECT_RESULT_SUCCESS == lv_spirvResult);


	}


	SpirvPipelineGenerator::~SpirvPipelineGenerator()
	{
		spvReflectDestroyShaderModule(&m_spvModule);
	}


	const std::vector<SpirvPipelineGenerator::VulkanDescriptorSetLayoutData>&
		SpirvPipelineGenerator::GenerateDescriptorSetLayouts()
	{

		for (size_t i = 0; i < m_setLayoutDatas.size(); ++i) {

			const SpvReflectDescriptorSet& lv_spvDescriptorSet = *m_spvDescriptorSets[i];

			m_setLayoutDatas[i].m_bindings.resize(lv_spvDescriptorSet.binding_count);

			for (size_t j = 0; j < m_setLayoutDatas[i].m_bindings.size(); ++j) {

				m_setLayoutDatas[i].m_bindings[j].binding = j;
				m_setLayoutDatas[i].m_bindings[j].descriptorType = (VkDescriptorType)(lv_spvDescriptorSet.bindings[j]->descriptor_type);
				m_setLayoutDatas[i].m_bindings[j].pImmutableSamplers = nullptr;

				//Easier to include both stages in order to avoid making the code more complicated
				m_setLayoutDatas[i].m_bindings[j].stageFlags = (0 != (((VkShaderStageFlags)m_spvModule.shader_stage) & VK_SHADER_STAGE_COMPUTE_BIT)) ? VK_SHADER_STAGE_COMPUTE_BIT : (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
				m_setLayoutDatas[i].m_bindings[j].descriptorCount = 1;

				for (size_t h = 0; h < lv_spvDescriptorSet.bindings[j]->array.dims_count; ++h) {
					m_setLayoutDatas[i].m_bindings[j].descriptorCount *= lv_spvDescriptorSet.bindings[j]->array.dims[h];
				}

			}

			m_setLayoutDatas[i].m_flags.resize(lv_spvDescriptorSet.binding_count);

			size_t j = 0;
			for (auto& l_flag : m_setLayoutDatas[i].m_flags) {

				if (m_setLayoutDatas[i].m_bindings[j].descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
					l_flag = 0;
				}
				else {
					if (m_setLayoutDatas[i].m_bindings[j].descriptorCount > 1) {
						l_flag = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
					}
					else {
						l_flag = 0;
					}
				}
			}

			m_setLayoutDatas[i].m_setLayoutBindingFlagsCreateInfo.bindingCount = m_setLayoutDatas[i].m_flags.size();
			m_setLayoutDatas[i].m_setLayoutBindingFlagsCreateInfo.pNext = nullptr;
			m_setLayoutDatas[i].m_setLayoutBindingFlagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
			m_setLayoutDatas[i].m_setLayoutBindingFlagsCreateInfo.pBindingFlags = m_setLayoutDatas[i].m_flags.data();


			m_setLayoutDatas[i].m_setNumber = lv_spvDescriptorSet.set;
			m_setLayoutDatas[i].m_setLayoutCreateInfo.bindingCount = lv_spvDescriptorSet.binding_count;
			m_setLayoutDatas[i].m_setLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
			m_setLayoutDatas[i].m_setLayoutCreateInfo.pBindings = m_setLayoutDatas[i].m_bindings.data();
			m_setLayoutDatas[i].m_setLayoutCreateInfo.pNext = &m_setLayoutDatas[i].m_setLayoutBindingFlagsCreateInfo;
			m_setLayoutDatas[i].m_setLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		}


		return m_setLayoutDatas;

	}


	void SpirvPipelineGenerator::DebugPipelineGenerator()
	{
		for (auto& l_setLayout : m_setLayoutDatas) {
			std::cout << "Number of bindings: " << l_setLayout.m_bindings.size() <<
				"\n" << "Set number: " << l_setLayout.m_setNumber << "\n\n" << std::endl;

			for (auto& l_binding : l_setLayout.m_bindings) {
				std::cout << "\n-----------\n" << "Binding number: " << l_binding.binding <<
					"\n" << "Descriptor count for this binding: " << l_binding.descriptorCount <<
					"\n" << std::endl;

				if (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER == l_binding.descriptorType) {
					std::cout << "Descriptor type: VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER\n" << std::endl;
				}
				if (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER == l_binding.descriptorType) {
					std::cout << "Descriptor type: VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER\n" << std::endl;
				}
				if (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER == l_binding.descriptorType) {
					std::cout << "Descriptor type: VK_DESCRIPTOR_TYPE_STORAGE_BUFFER\n" << std::endl;
				}
			}

		}

	}


}