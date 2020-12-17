#pragma once

#include "Pch.h"
#include "WindowEvents.h"

class Window : public WindowListener
{
public:
	Window() = default;
	virtual ~Window();

	bool Create(std::string&& title, int width, int height);
	void Destroy();

private:
	SDL_Window* m_Window = nullptr;

	// Inherited via WindowListener
	virtual void OnResize() override;
};