#pragma once

#include "Pch.h"

class WindowListener
{
public:
	WindowListener() = default;
	virtual ~WindowListener() = default;
	virtual void OnResize(int width, int height) = 0;
};

class QuitListener
{
public:
	QuitListener() = default;
	virtual ~QuitListener() = default;
	virtual void OnQuit() = 0;
};

class KeyboardListener
{
public:
	KeyboardListener() = default;
	virtual ~KeyboardListener() = default;

	virtual void OnKeyPressed(SDL_Scancode scancode) = 0;
};

class EventDispatcher
{
public:
	EventDispatcher() = default;
	virtual ~EventDispatcher() = default;

	void Poll();

	void Attach(WindowListener* listener) { m_WindowListeners.push_back(listener); }
	void Attach(QuitListener* listener) { m_QuitListener.push_back(listener); }
	void Attach(KeyboardListener* listener) { m_KeyboardListener.push_back(listener); }

private:
	// Window events
	std::vector<WindowListener*> m_WindowListeners;
	void PollWindowEvents(const SDL_Event& e);

	// Quit events
	std::vector<QuitListener*> m_QuitListener;
	void PollQuitEvents();

	// Keyboard events
	std::vector<KeyboardListener*> m_KeyboardListener;
	void PollKeyboardEvents(const SDL_Event& e);
};
