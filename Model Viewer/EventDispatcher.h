#pragma once

#include "Pch.h"

class WindowListener;

class QuitListener
{
public:
	QuitListener() = default;
	virtual ~QuitListener() = default;
	virtual void OnQuit() = 0;
};

class EventDispatcher
{
public:
	EventDispatcher() = default;
	virtual ~EventDispatcher() = default;

	void Poll();

	void Attach(WindowListener* listener);
	void Attach(QuitListener* listener);

private:
	std::vector<WindowListener*> m_WindowListeners;
	void PollWindowEvents(SDL_Event& e);

	std::vector<QuitListener*> m_QuitListener;
	void PollQuitEvents();
};
