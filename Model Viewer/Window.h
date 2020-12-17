#pragma once

#include "Pch.h"

class Window
{
public:
	Window() = default;
	virtual ~Window();

	bool Create(std::string&& title, int width, int height);
	void Destroy();

	constexpr SDL_Window* GetSdlWindow() { return m_Window; }

	int GetWidth() const;
	int GetHeight() const;

private:
	SDL_Window* m_Window = nullptr;
};