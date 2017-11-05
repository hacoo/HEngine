#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Image::PixelType Image::getType()
{
	return type;
}

STB_RGBA_Image::STB_RGBA_Image()
		:
	Image(Image::PixelType::RGBA),
	height(0),
	width(0),
	components(0),
	pixels(nullptr)
{ }

STB_RGBA_Image::STB_RGBA_Image(const std::string& load_path)
	:
	Image(Image::PixelType::RGBA)
{
	int w, h, c;

	// Set req_comp=4, we want RGBA
	pixels = stbi_load(load_path.c_str(), &w, &h, &c, 4);

	if (!pixels)
	{
		throw std::runtime_error("Failed to load image at path: " + load_path);
	}

	width = static_cast<size_t>(w);
	height = static_cast<size_t>(h);
	components = static_cast<size_t>(c);
}

STB_RGBA_Image::STB_RGBA_Image(const STB_RGBA_Image& other)
	:
	Image(other.type),
	height(other.height),
	width(other.width),
	components(other.components)
{
	int h, w, c;
	pixels = stbi_load_from_memory((const stbi_uc*)other.pixels, (int)width * (int)height * (int)components, &w, &h, &c, 4);
	width = static_cast<size_t>(w);
	height = static_cast<size_t>(h);
	components = static_cast<size_t>(c);

}

STB_RGBA_Image::STB_RGBA_Image(STB_RGBA_Image&& rhs)
	:
	Image(rhs.type),
	height(rhs.height),
	width(rhs.width),
	components(rhs.components)
{
	rhs.height = 0;
	rhs.width = 0;
	rhs.components = 0;
	pixels = rhs.pixels;
	rhs.pixels = nullptr;
}

STB_RGBA_Image& STB_RGBA_Image::operator=(const STB_RGBA_Image& other)
{
	height = other.height;
	width = other.width;
	components = other.components;

	int h, w, c;
	pixels = stbi_load_from_memory((const stbi_uc*)other.pixels, (int)width * (int)height * (int)components, &w, &h, &c, 0);

	width = static_cast<size_t>(w);
	height = static_cast<size_t>(h);
	components = static_cast<size_t>(c);

	return *this;
}

STB_RGBA_Image& STB_RGBA_Image::operator=(STB_RGBA_Image&& rhs)
{
	height = rhs.height;
	width = rhs.width;
	components = rhs.components;

	rhs.height = 0;
	rhs.width = 0;
	rhs.components = 0;

	void* temp = pixels;
	pixels = rhs.pixels;
	rhs.pixels = temp;

	return *this;
}
	
STB_RGBA_Image::~STB_RGBA_Image()
{
	if (pixels != nullptr)
	{
		stbi_image_free(pixels);
	}
}

void* STB_RGBA_Image::data()
{
	return (void*) pixels;
}
