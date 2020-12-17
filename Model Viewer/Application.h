#pragma once

#include "EventDispatcher.h"
#include "Window.h"
#include "Renderer.h"

#include "WindowEvents.h"

class Application : public QuitListener, public WindowListener
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
	std::unique_ptr<Renderer> m_Renderer;

	// Inherited via QuitListener
	virtual void OnQuit() override;

	// Inherited via WindowListener
	virtual void OnResize(int width, int height) override;
};