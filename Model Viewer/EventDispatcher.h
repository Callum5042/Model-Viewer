#pragma once

#include "Pch.h"

struct MouseData
{
	int x = 0;
	int y = 0;
	int xrel = 0;
	int yrel = 0;
	Uint32 state = 0;
};

class WindowListener
{
public:
	WindowListener() = default;
	virtual ~WindowListener() = default;
	virtual void OnResize(int width, int height) {}
};

class QuitListener
{
public:
	QuitListener() = default;
	virtual ~QuitListener() = default;
	virtual void OnQuit() {};
};

class KeyboardListener
{
public:
	KeyboardListener() = default;
	virtual ~KeyboardListener() = default;

#pragma warning(disable : 26812)
	virtual void OnKeyPressed(SDL_Scancode scancode) {};
};

class MouseListener
{
public:
	MouseListener() = default;
	virtual ~MouseListener() = default;

	virtual void OnMouseMove(const MouseData& mouse) {};
	virtual void OnMousePressed(const MouseData& mouse) {};
	virtual void OnMouseReleased(const MouseData& mouse) {};
	virtual void OnMouseWheel(const MouseData& mouse) {};
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
	void Attach(MouseListener* listener) { m_MouseListener.push_back(listener); }

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

	// Mouse events
	std::vector<MouseListener*> m_MouseListener;
	void PollMouseEvents(const SDL_Event& e);
};
