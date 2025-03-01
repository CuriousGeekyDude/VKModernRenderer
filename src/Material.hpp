#pragma once


#include <glm/glm.hpp>



namespace SceneConverter
{

	enum class MaterialFlags : uint32_t
	{
		sMaterialFlags_CastShadow = 0,
		sMaterialFlags_ReceiveShadow = 1,
		sMaterialFlags_Transparent = 2,
		sMaterialFlags_AmbientOcclusionMapIncluded = 4,
		sMaterialFlags_EmissiveMapIncluded = 8,
		sMaterialFlags_AlbedoMapIncluded = 16,
		sMaterialFlags_MetallicRoughnessMapIncluded = 32,
		sMaterialFlags_NormalMapIncluded = 64,
		sMaterialFlags_OpacityMapIncluded = 128,
		sMaterialFlags_MetallicMapIncluded = 256,
		sMaterialFlags_RoughnessMapIncluded = 512
	};

	constexpr const int INVALID_TEXTURE = 0xFFFFFFFF;

	struct alignas(16) Material final
	{
		glm::vec4 m_emissiveColor{};
		glm::vec4 m_albedo{1.f, 1.f, 1.f, 1.f};
		glm::vec4 m_roughness{1.f, 1.f, 0.f, 0.f};
		glm::vec4 m_specular{0.5f, 0.5f, 0.5f, 0.f};

		float m_transparencyFactor = 1.0f;
		float m_alphaTest = 0.0f;
		float m_metallicFactor = 1.0f;
		
		uint32_t m_flags{};

		int m_ambientOcclusionMap = INVALID_TEXTURE;
		int m_emissiveMap = INVALID_TEXTURE;
		int m_albedoMap = INVALID_TEXTURE;
		int m_metallicRoughnessMap = INVALID_TEXTURE;
		int m_normalMap = INVALID_TEXTURE;
		int m_opacityMap = INVALID_TEXTURE;
		int m_metallicMap{};
		int m_roughnessMap{};
	};
}