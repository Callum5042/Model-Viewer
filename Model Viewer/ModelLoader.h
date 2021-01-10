#pragma once

#include <string>
#include "Model.h"

namespace ModelLoader
{
	bool Load(const std::string& path, MeshData* meshData);
}