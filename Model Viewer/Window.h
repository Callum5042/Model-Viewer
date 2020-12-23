#pragma once

#include "Pch.h"


enum class WindowMode
{
	WINDOW,
	MAXIMISED,
	FULLSCREEN,
	BORDERLESS_FULLSCREEN,
};

class Window
{
public:
	Window() = default;
	virtual ~Window();

	virtual bool Create(std::string&& title, int width, int height, WindowMode windowMode);
	void Destroy();

	constexpr SDL_Window* GetSdlWindow() { return m_Window; }

	// Window dimensions
	int GetWidth() const;
	int GetHeight() const;
	Window& SetWidth(int width);
	Window& SetHeight(int height);

	// Window position
	void SetPosition(int x, int y);
	void GetPosition(int* x, int* y);

	//[[deprecated]]
	WindowMode GetWindowMode() { return m_WindowMode; }

	// Window maximised
	bool IsMaximised() const;
	void Maximise();

protected:
	SDL_Window* m_Window = nullptr;
	WindowMode m_WindowMode = WindowMode::WINDOW;
};

class OpenGLWindow : public Window
{
public:
	OpenGLWindow() = default;
	virtual ~OpenGLWindow();

	bool Create(std::string&& title, int width, int height, WindowMode windowMode) override;

	constexpr SDL_GLContext GetOpenGLContext() { return m_OpenGlContext; }

protected:
	SDL_GLContext m_OpenGlContext = nullptr;
};