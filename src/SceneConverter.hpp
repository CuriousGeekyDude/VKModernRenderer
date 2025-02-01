#pragma once 


#include "Scene.hpp"
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <string>

namespace SceneConverter
{
	class SceneConverter final
	{
	public:
		
		SceneConverter(const aiScene* l_assimpScene);

		Scene& GetScene() { return m_scene; }

	private:

		uint32_t AddNodeToScene(const uint32_t l_level, const int l_parent);
		void TraverseAssimpScene(const uint32_t l_level, const int l_parent, 
			const aiNode* l_assimpNode);

		void ConverteAssimpMatrixToGlmMatrix(const aiMatrix4x4& l_assimpMatrix, glm::mat4& l_glmMatrix);

	private:
		
		Scene m_scene{};
		const aiScene* m_assimpScene{};

	};
}