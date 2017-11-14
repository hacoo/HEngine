
/* 
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>  
#include <crtdbg.h>  
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif
*/

#include <iostream>
#include <stdexcept>
#include <VulkanApplication.h>

int launchVulkanApplication()
{
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
		return -1;
	}

	return 0;
}

int main() 
{
	/*
#ifdef _DEBUG
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); 
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif _DEBUG
	*/

	int retcode = launchVulkanApplication();
	
	/*
#ifdef _DEBUG	
	_CrtDumpMemoryLeaks();
#endif
	*/

	std::cout << "Shutdown complete. " << std::endl;

	Sleep(5000);

	return retcode == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}