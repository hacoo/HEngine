
// Basic types used in rendering

#pragma once

#include <glm/glm.hpp>

// Render-able 2d vertex
struct Vertex2D
{
	glm::vec2 pos;
	glm::vec3 color;	
	glm::vec2 texCoord;
};
