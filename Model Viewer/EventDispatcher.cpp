#include "Pch.h"
#include "EventDispatcher.h"
#include "WindowEvents.h"
#include "imgui_impl_sdl.h"

void EventDispatcher::Poll()
{
	SDL_Event e = {};
	while (SDL_PollEvent(&e))
	{
		ImGui_ImplSDL2_ProcessEvent(&e);

		switch (e.type)
		{
		case SDL_QUIT:
			PollQuitEvents();
			break;

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

void EventDispatcher::Attach(QuitListener* listener)
{
	m_QuitListener.push_back(listener);
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
					listener->OnResize(e.window.data1, e.window.data2);
				}
			}
		}
	}
}

void EventDispatcher::PollQuitEvents()
{
	for (auto& listener : m_QuitListener)
	{
		if (listener != nullptr)
		{
			listener->OnQuit();
		}
	}
}
