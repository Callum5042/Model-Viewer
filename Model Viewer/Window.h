#pragma once

#include "Pch.h"
#include "WindowEvents.h"

class Window : public WindowListener
{
public:
	Window() = default;
	virtual ~Window() = default;

	bool Create(std::string&& title, int width, int height);

private:
	SDL_Window* m_Window = nullptr;

	// Inherited via WindowListener
	virtual void OnResize() override;
};