#include "Pch.h"
#include <iostream>

#include "EventDispatcher.h"
#include "Window.h"

int main(int argc, char** argv)
{
	EventDispatcher eventDispatcher;

	Window window;
	if (!window.Create("Test", 800, 600))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Window::Create failed!", nullptr);
		return -1;
	}

	while (true)
	{
		eventDispatcher.Poll();
	}

	return 0;
}