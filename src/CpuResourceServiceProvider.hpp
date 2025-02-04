#pragma once




#include <cinttypes>
#include <unordered_map>
#include <string>


namespace VulkanEngine
{

	struct CpuResource
	{
		void* m_buffer = nullptr;
		uint32_t m_size{UINT32_MAX};
	};

	class CpuResourceServiceProvider
	{
	public:


		void AddCpuResource(const std::string& l_resourceName
			,void* l_data, uint32_t l_size);
		CpuResource RetrieveCpuResource(const std::string& l_resourceName);


	private:
		std::unordered_map<std::string, CpuResource> m_mappedNameToData;
	};

}