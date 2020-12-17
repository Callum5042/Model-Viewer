#include "Pch.h"
#include "Application.h"

#ifdef _WIN32
#include <crtdbg.h>
#endif

int main(int argc, char** argv)
{
#ifdef _WIN32
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	std::unique_ptr<Application> application = std::make_unique<Application>();
	return application->Execute();
}