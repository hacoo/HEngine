// Utilities for dealing with .obj files

#pragma once

#include <vector>
#include <string>

#include "render_types.h"



namespace ObjUtil
{
	bool loadObj(std::string path, std::vector<Vertex3D>& outVertices, std::vector<uint32_t>& outIndices);
};