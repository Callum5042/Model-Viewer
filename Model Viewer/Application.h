#pragma once

#include "EventDispatcher.h"
#include "Window.h"

class Application : public QuitListener
{
public:
	Application();
	virtual ~Application();

	int Execute();

private:
	bool m_Running = true;

	bool Init();

	std::unique_ptr<Window> m_Window;
	std::unique_ptr<EventDispatcher> m_EventDispatcher;

	// Inherited via QuitListener
	virtual void OnQuit() override;
};