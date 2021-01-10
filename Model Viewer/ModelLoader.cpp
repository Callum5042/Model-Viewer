#include "Pch.h"
#include "ModelLoader.h"

#undef min
#undef max
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace
{
	DirectX::XMMATRIX ConvertToDirectXMatrix(aiMatrix4x4 matrix)
	{
		aiVector3D scale, rot, pos;
		matrix.Decompose(scale, rot, pos);

		DirectX::XMMATRIX _matrix = DirectX::XMMatrixIdentity();
		_matrix *= DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
		_matrix *= DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
		_matrix *= DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);

		return _matrix;
	}

	void LoadVertices(aiMesh* mesh, MeshData* meshData, unsigned& vertex_count)
	{
		for (auto i = 0u; i < mesh->mNumVertices; ++i)
		{
			vertex_count++;

			// Set the positions
			float x = static_cast<float>(mesh->mVertices[i].x);
			float y = static_cast<float>(mesh->mVertices[i].y);
			float z = static_cast<float>(mesh->mVertices[i].z);

			// Create a vertex to store the mesh's vertices temporarily
			Vertex vertex;
			vertex.position.x = x;
			vertex.position.y = y;
			vertex.position.z = z;

			// Detect and write colours
			if (mesh->HasVertexColors(0))
			{
				auto assimp_colour = mesh->mColors[0][i];

				vertex.colour.r = assimp_colour.r;
				vertex.colour.g = assimp_colour.g;
				vertex.colour.b = assimp_colour.b;
				vertex.colour.a = assimp_colour.a;
			}

			// Detect and write normals
			if (mesh->HasNormals())
			{
				vertex.normal.x = mesh->mNormals[i].x;
				vertex.normal.y = mesh->mNormals[i].y;
				vertex.normal.z = mesh->mNormals[i].z;
			}

			// Has texture UV's
			if (mesh->mTextureCoords[0])
			{
				vertex.texture.u = mesh->mTextureCoords[0][i].x;
				vertex.texture.v = mesh->mTextureCoords[0][i].y;
			}

			// Detect and write tangents and bi-tangents
			if (mesh->HasTangentsAndBitangents())
			{
				vertex.tangent.x = mesh->mTangents[i].x;
				vertex.tangent.y = mesh->mTangents[i].y;
				vertex.tangent.z = mesh->mTangents[i].z;

				vertex.bi_tangent.x = mesh->mBitangents[i].x;
				vertex.bi_tangent.y = mesh->mBitangents[i].y;
				vertex.bi_tangent.z = mesh->mBitangents[i].z;
			}

			// Add the vertex to the vertices vector
			meshData->vertices.push_back(vertex);
		}
	}

	void LoadIndices(aiMesh* mesh, MeshData* meshData, unsigned& index_count)
	{
		for (auto i = 0u; i < mesh->mNumFaces; ++i)
		{
			// Get the face
			const auto& face = mesh->mFaces[i];

			// Add the indices of the face to the vector
			for (auto k = 0u; k < face.mNumIndices; ++k)
			{
				index_count++;
				meshData->indices.push_back(face.mIndices[k]);
			}
		}
	}
}

bool ModelLoader::Load(const std::string& path, MeshData* meshData)
{
	Assimp::Importer importer;
	auto scene = importer.ReadFile(path, aiProcessPreset_TargetRealtime_Fast | aiProcess_ConvertToLeftHanded | aiProcess_PopulateArmatureData);

	// Load model
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << '\n';
		return false;
	}

	auto index_count_total = 0u;
	auto vertex_count_total = 0u;

	// Iterate over the meshes
	meshData->subsets.resize(scene->mNumMeshes);
	for (auto mesh_index = 0u; mesh_index < scene->mNumMeshes; ++mesh_index)
	{
		auto mesh = scene->mMeshes[mesh_index];

		Subset subset;
		subset.startIndex = index_count_total;
		subset.baseVertex = vertex_count_total;

		LoadVertices(mesh, meshData, vertex_count_total);

		auto index_count = 0u;
		LoadIndices(mesh, meshData, index_count);

		index_count_total += index_count;
		subset.totalIndex = index_count;
		meshData->subsets[mesh_index] = subset;

		// Load bones
		for (auto bone_index = 0u; bone_index < mesh->mNumBones; ++bone_index)
		{
			auto ai_bone = mesh->mBones[bone_index];

			BoneInfo boneInfo = {};
			boneInfo.name = ai_bone->mName.C_Str();
			boneInfo.parentName = ai_bone->mNode->mParent->mName.C_Str();
			meshData->bones.push_back(boneInfo);

			meshData->bones[bone_index].offset = ConvertToDirectXMatrix(ai_bone->mOffsetMatrix);;

			// Vertex weight data
			for (auto bone_weight_index = 0u; bone_weight_index < ai_bone->mNumWeights; bone_weight_index++)
			{
				auto vertexID = ai_bone->mWeights[bone_weight_index].mVertexId;
				auto weight = ai_bone->mWeights[bone_weight_index].mWeight;
				auto& vertex = meshData->vertices[vertexID];

				for (int vertex_weight_index = 0; vertex_weight_index < 4; ++vertex_weight_index)
				{
					if (vertex.weight[vertex_weight_index] == 0.0)
					{
						vertex.weight[vertex_weight_index] = weight;
						vertex.bone[vertex_weight_index] = bone_index;
						break;
					}
				}
			}
		}

		// Calculate parent
		for (int i = 0; i < meshData->bones.size(); ++i)
		{
			int parentId = 0;
			for (int j = 0; j < meshData->bones.size(); ++j)
			{
				if (meshData->bones[i].parentName == meshData->bones[j].name)
				{
					parentId = j;
					break;
				}
			}

			meshData->bones[i].parentId = parentId;
		}
	}

	// Load animations
	for (auto animation_index = 0u; animation_index < scene->mNumAnimations; ++animation_index)
	{
		auto animation = scene->mAnimations[animation_index];
		auto ticksPerSecond = static_cast<float>(animation->mTicksPerSecond);

		AnimationClip clip;
		clip.BoneAnimations.resize(animation->mNumChannels);
		for (unsigned i = 0; i < animation->mNumChannels; ++i)
		{
			auto channel = animation->mChannels[i];
			std::string name = channel->mNodeName.C_Str();
			for (unsigned k = 0; k < channel->mNumPositionKeys; ++k)
			{
				auto time = channel->mPositionKeys[k].mTime;
				auto pos = channel->mPositionKeys[k].mValue;
				auto rotation = channel->mRotationKeys[k].mValue;
				auto scale = channel->mScalingKeys[k].mValue;

				Keyframe frame;
				frame.TimePos = static_cast<float>(time);
				frame.Translation = DirectX::XMFLOAT3(pos.x, pos.y, pos.z);
				frame.RotationQuat = DirectX::XMFLOAT4(rotation.x, rotation.y, rotation.z, rotation.w);
				frame.Scale = DirectX::XMFLOAT3(scale.x, scale.y, scale.z);

				clip.BoneAnimations[i].Keyframes.push_back(frame);
			}
		}

		std::string animation_name = animation->mName.C_Str();
		meshData->animations["Take1"] = clip;
	}

	return true;
}