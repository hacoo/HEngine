
// Basic types used in rendering

#pragma once

#include <glm/glm.hpp>

// Render-able 3D vertex
struct Vertex3D
{
	glm::vec3 pos;
	glm::vec3 color;	
	glm::vec2 texCoord;
};