
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>  
#include <crtdbg.h>  
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#include <iostream>
#include <stdexcept>
#include <VulkanApplication.h>

int main() 
{

#ifdef _DEBUG
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); 
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif _DEBUG

	VulkanApplication app;

	try
	{
		app.run();
	}
	catch (const std::runtime_error& err)
	{
		std::cout << "UNHANDLED EXCEPTION" << std::endl;
		std::cout << err.what() << std::endl;
		OutputDebugString(LPCWSTR("UNHANDLED EXCEPTION\n"));
		OutputDebugString(LPCWSTR(err.what()));
		OutputDebugString(LPCWSTR("\n"));
		return EXIT_FAILURE;
	}
	
#ifdef _DEBUG	
	_CrtDumpMemoryLeaks();
#endif

	return EXIT_SUCCESS;
}