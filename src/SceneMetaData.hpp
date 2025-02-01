#pragma once


#include <string>


namespace SceneMetaData
{
	struct SceneMetaData
	{
		std::string m_assimpSceneFileName;

		//Mesh file header file to load and save from
		// which containes the raw binary data customized MeshFileHeader structure and related data
		std::string m_outputMesh;

		//Scene file to load and save from which contains 
		// raw binary data of the customized Scene structures
		std::string m_outputScene;

		//Material file to load and save from
		// which contains the raw binary data of the customized Material structures
		std::string m_outputMaterials;

		//Bounding box file to load and save from
		// which contains the raw binary data of the customized BoundingBox structure
		std::string m_outputBoxes;


		//Instance data file to load and save from 
		// which contains the raw binary data of the customized InstanceData structure
		std::string m_outputInstanceData;

		float m_scale;
		bool m_mergeInstances;
	};
}