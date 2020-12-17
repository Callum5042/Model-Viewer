#include "Pch.h"
#include "EventDispatcher.h"
#include "WindowEvents.h"

std::vector<WindowListener*> EventDispatcher::m_WindowListeners;

void EventDispatcher::Poll()
{
	SDL_Event e = {};
	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
		case SDL_WINDOWEVENT:
			PollWindowEvents(e);
			break;
		}
	}
}

void EventDispatcher::Attach(WindowListener* listener)
{
	m_WindowListeners.push_back(listener);
}

void EventDispatcher::PollWindowEvents(SDL_Event& e)
{
	if (e.type == SDL_WINDOWEVENT)
	{
		if (e.window.event == SDL_WINDOWEVENT_RESIZED)
		{
			for (auto& listener : m_WindowListeners)
			{
				if (listener != nullptr)
				{
					listener->OnResize();
				}
			}
		}
	}
}