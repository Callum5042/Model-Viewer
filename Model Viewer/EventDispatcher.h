#pragma once

#include "Pch.h"

class WindowListener;

class EventDispatcher
{
public:
	static void Poll();

	static void Attach(WindowListener* listener);

private:
	static std::vector<WindowListener*> m_WindowListeners;
	static void PollWindowEvents(SDL_Event& e);
};
