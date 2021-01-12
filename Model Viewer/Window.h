#pragma once

#include "Pch.h"

// Window state
enum class WindowMode
{
	WINDOW,
	MAXIMISED,
	FULLSCREEN,
	BORDERLESS_FULLSCREEN,
};

// Main DirectX Window
class Window
{
public:
	Window() noexcept = default;
	virtual ~Window();
	Window& operator=(const Window&) = delete;
	Window(const Window&) = delete;

	// Creates the window
	virtual bool Create(std::string&& title, int width, int height, WindowMode windowMode);

	// Get the underlying SDL Window handle
	constexpr SDL_Window* GetSdlWindow() { return m_Window; }

	// Get the window width in pixels
	int GetWidth() const;

	// Get the window height in pixels
	int GetHeight() const;

	// Set the window width in pixels
	Window& SetWidth(int width);

	// Set the window height in pixels
	Window& SetHeight(int height);

	// Set window position relative to the top-left corner
	void SetPosition(int x, int y);

	// Get window position relative to the top-left corner
	void GetPosition(int* x, int* y);

	// Get window mode
	WindowMode GetWindowMode() { return m_WindowMode; }

	// Get if window is currently maximised
	bool IsMaximised() const;

	// Maximise the winndow
	void Maximise();

protected:
	SDL_Window* m_Window = nullptr;
	WindowMode m_WindowMode = WindowMode::WINDOW;
};

// OpenGL Window. Requires additional window setup code
class GLWindow : public Window
{
public:
	GLWindow() noexcept = default;
	virtual ~GLWindow();
	GLWindow& operator=(const GLWindow&) = delete;
	GLWindow(const GLWindow&) = delete;

	// Creates the window
	virtual bool Create(std::string&& title, int width, int height, WindowMode windowMode) override;

	// Get the SDL OpenGL context
	constexpr SDL_GLContext GetOpenGLContext() { return m_OpenGlContext; }

protected:
	SDL_GLContext m_OpenGlContext = nullptr;
};