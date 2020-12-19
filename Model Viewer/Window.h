#pragma once

#include "Pch.h"

enum class WindowMode
{
	WINDOW,
	FULLSCREEN,
	BORDERLESS_FULLSCREEN
};

class Window
{
public:
	Window() = default;
	virtual ~Window();

	virtual bool Create(std::string&& title, int width, int height, WindowMode windowMode);
	void Destroy();

	constexpr SDL_Window* GetSdlWindow() { return m_Window; }

	int GetWidth() const;
	int GetHeight() const;

	WindowMode GetWindowMode() { return m_WindowMode; }

protected:
	SDL_Window* m_Window = nullptr;
	WindowMode m_WindowMode = WindowMode::WINDOW;
};

class OpenGLWindow : public Window
{
public:
	OpenGLWindow() = default;
	virtual ~OpenGLWindow();

	bool Create(std::string&& title, int width, int height, WindowMode windowMode);

	constexpr SDL_GLContext GetOpenGLContext() { return m_OpenGlContext; }

protected:
	SDL_GLContext m_OpenGlContext = nullptr;
};