#include "Pch.h"
#include "ModelLoader.h"

#undef min
#undef max
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace
{
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
	}

	return true;
}