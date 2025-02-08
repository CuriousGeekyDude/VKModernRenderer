



#include "CpuResourceServiceProvider.hpp"


namespace VulkanEngine
{


	void CpuResourceServiceProvider::AddCpuResource(const std::string& l_resourceName
		,void* l_data, uint32_t l_size)
	{
		m_mappedNameToData.emplace(l_resourceName, CpuResource{ l_data, l_size });

	}
	VulkanEngine::CpuResource CpuResourceServiceProvider::RetrieveCpuResource(const std::string& l_resourceName)
	{

		CpuResource lv_resource;

		if (m_mappedNameToData.end() != m_mappedNameToData.find(l_resourceName)) {
			lv_resource = m_mappedNameToData[l_resourceName];
		}

		return lv_resource;

	}

}