/* Defines Image class, representing an image as raw pixels */

#pragma once

#include<string>

// Abstract base class 
class Image
{
public:
	enum PixelType
	{
		RGBA
	};

public:
	Image(Image::PixelType _type)
		:
		type(_type)
	{ }

	virtual ~Image() { }
	virtual Image::PixelType getType();

protected:
	Image::PixelType type;
};

class STB_RGBA_Image : public Image
{
public:
	STB_RGBA_Image();
	// Loads image from the specified path. If load fails, will throw std::runtime_error
	STB_RGBA_Image(const std::string& load_path);
	STB_RGBA_Image(const STB_RGBA_Image& other);
	STB_RGBA_Image(STB_RGBA_Image&& rhs);
	STB_RGBA_Image& operator=(const STB_RGBA_Image& other);
	STB_RGBA_Image& operator=(STB_RGBA_Image&& other);
	virtual ~STB_RGBA_Image();

	void* data();

public: 
	size_t height;
	size_t width;
	size_t components;
	void* pixels;
};