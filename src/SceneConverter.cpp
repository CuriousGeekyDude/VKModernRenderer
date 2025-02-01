


#include "SceneConverter.hpp"


namespace SceneConverter
{

	SceneConverter::SceneConverter(const aiScene* l_assimpScene) : m_assimpScene(l_assimpScene)
	{
		TraverseAssimpScene(0, -1, l_assimpScene->mRootNode);

	}


	uint32_t SceneConverter::AddNodeToScene(const uint32_t l_level, const int l_parent)
	{
		uint32_t lv_newNodeIndex = m_scene.m_hierarchies.size();

		Hierarchy lv_hierarchyNewNode{};
		lv_hierarchyNewNode.m_level = l_level;
		lv_hierarchyNewNode.m_parent = l_parent;
		lv_hierarchyNewNode.m_nextSibling = -1;
		lv_hierarchyNewNode.m_firstChild = -1;
		lv_hierarchyNewNode.m_lastSibling = -1;
		m_scene.m_hierarchies.push_back(lv_hierarchyNewNode);

		m_scene.m_globalTransforms.push_back(glm::mat4(1.f));
		m_scene.m_localTransforms.push_back(glm::mat4(1.f));

		if (0 <= l_parent) {

			auto& lv_parentHierarchy = m_scene.m_hierarchies[l_parent];

			if (0 < lv_parentHierarchy.m_firstChild) {

				auto& lv_firstChildHierarchy = m_scene.m_hierarchies[lv_parentHierarchy.m_firstChild];
				auto& lv_oldLastSiblingHierarchy = m_scene.m_hierarchies[lv_firstChildHierarchy.m_lastSibling];

				lv_firstChildHierarchy.m_lastSibling = lv_newNodeIndex;
				lv_oldLastSiblingHierarchy.m_nextSibling = lv_newNodeIndex;

			}
			else {
				lv_parentHierarchy.m_firstChild = lv_newNodeIndex;
				m_scene.m_hierarchies[lv_newNodeIndex].m_lastSibling = lv_newNodeIndex;
			}
		}
		
		return lv_newNodeIndex;

	}


	void SceneConverter::ConverteAssimpMatrixToGlmMatrix(const aiMatrix4x4& l_assimpMatrix,
		glm::mat4& l_glmMatrix)
	{
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				l_glmMatrix[i][j] = l_assimpMatrix[i][j];
			}
		}
	}


	void SceneConverter::TraverseAssimpScene(const uint32_t l_level, const int l_parent,
		const aiNode* l_assimpNode)
	{
		auto lv_newNodeIndex = AddNodeToScene(l_level, l_parent);

		ConverteAssimpMatrixToGlmMatrix(l_assimpNode->mTransformation, m_scene.m_localTransforms[lv_newNodeIndex]);

		for (uint32_t i = 0; i < l_assimpNode->mNumMeshes; ++i) {

			auto lv_newSubnodeIndex = AddNodeToScene(l_level + 1, lv_newNodeIndex);

			m_scene.m_meshes[lv_newSubnodeIndex] = l_assimpNode->mMeshes[i];

			m_scene.m_materialIDs[lv_newSubnodeIndex] = m_assimpScene->mMeshes[l_assimpNode->mMeshes[i]]->mMaterialIndex;

		}

		for (uint32_t i = 0; i < l_assimpNode->mNumChildren; ++i) {
			TraverseAssimpScene(l_level + 1, lv_newNodeIndex, l_assimpNode->mChildren[i]);
		}
	}
}