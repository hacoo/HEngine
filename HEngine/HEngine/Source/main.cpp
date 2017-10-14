
#include <iostream>
#include <HelloTriangleApplication.h>

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>  
#include <crtdbg.h>  

#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)

#endif

int main() 
{

#ifdef _DEBUG
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); 
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif _DEBUG

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
	
#ifdef _DEBUG	
	_CrtDumpMemoryLeaks();
#endif

	return EXIT_SUCCESS;
}