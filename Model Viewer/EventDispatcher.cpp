#include "Pch.h"
#include "EventDispatcher.h"
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

		case SDL_KEYDOWN:
			PollKeyboardEvents(e);
			break;

		case SDL_MOUSEMOTION:
			PollMouseEvents(e);
			break;
		}
	}
}

void EventDispatcher::PollWindowEvents(const SDL_Event& e)
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

void EventDispatcher::PollKeyboardEvents(const SDL_Event& e)
{
	if (e.type == SDL_KEYDOWN)
	{
		if (e.key.repeat == 0)
		{
			for (auto& listener : m_KeyboardListener)
			{
				if (listener != nullptr)
				{
					listener->OnKeyPressed(e.key.keysym.scancode);
				}
			}
		}
	}
}

void EventDispatcher::PollMouseEvents(const SDL_Event& e)
{
	MouseData data;
	data.x = e.motion.x;
	data.y = e.motion.y;
	data.xrel = e.motion.xrel;
	data.yrel = e.motion.yrel;
	data.state = e.motion.state;

	if (e.type == SDL_MOUSEMOTION)
	{
		for (auto& listener : m_MouseListener)
		{
			if (listener != nullptr)
			{
				listener->OnMouseMove(std::move(data));
			}
		}
	}
}
