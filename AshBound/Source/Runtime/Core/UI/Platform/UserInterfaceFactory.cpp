#include "Runtime/Core/UI/UserInterface.h"

#if defined(_WIN32)
#include "Runtime/Core/UI/Platform/WindowsImGuiInterface.h"
#endif

std::unique_ptr<UserInterface> CreateUserInterfaceForPlatform()
{
#if defined(_WIN32)
	return std::make_unique<WindowsImGuiInterface>();
#else
	return nullptr;
#endif
}
