
#include <iostream>
#include <HelloTriangleApplication.h>

int main() 
{
	HelloTriangleApplication app;

	try
	{
		app.run();
	}
	catch (const std::runtime_error& err)
	{
		std::cerr << err.what() << std::endl;
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}