#include "obj_util.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace ObjUtil
{
	bool loadObj(std::string path, std::vector<Vertex3D>& outVertices, std::vector<uint32_t>& outIndices)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err;
		
		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path.c_str())) 
		{
			return false;
		}

		outVertices.reserve(shapes.size() * 3);
		outIndices.reserve(shapes.size());

		for (const auto& shape : shapes) 
		{
			for (const auto& index : shape.mesh.indices) 
			{
				Vertex3D vertex = {};

				vertex.pos = {
attrib.vertices[3 * index.vertex_index + 0],
attrib.vertices[3 * index.vertex_index + 1],
attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.texCoord = {
attrib.texcoords[2 * index.texcoord_index + 0],
1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

				vertex.color = { 1.0f, 1.0f, 1.0f };

				outVertices.push_back(vertex);
				outIndices.push_back(static_cast<uint32_t>(outIndices.size()));
			}
		}

		return true;
	}
};