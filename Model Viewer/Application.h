#pragma once

#include "EventDispatcher.h"
#include "Window.h"
#include "Renderer.h"
#include "Timer.h"
#include "Model.h"
#include "Camera.h"
#include "Shader.h"

class Application : public QuitListener, public WindowListener
{
public:
	Application();
	virtual ~Application();

	int Execute();

private:
	bool m_Running = true;
	Timer m_Timer;
	int m_FramesPerSecond = 0;
	void CalculateFramesPerSecond();

	bool Init();
	void Render();
	void RenderGui();

	std::unique_ptr<Window> m_Window = nullptr;
	std::unique_ptr<EventDispatcher> m_EventDispatcher = nullptr;
	std::unique_ptr<IRenderer> m_Renderer = nullptr;

	void ChangeRenderAPI();
	RenderAPI m_SwitchRenderAPI = RenderAPI::NONE;

	std::unique_ptr<IModel> m_Model = nullptr;
	std::unique_ptr<Camera> m_Camera = nullptr;
	std::unique_ptr<GlCamera> m_GlCamera = nullptr;
	std::unique_ptr<IShader> m_Shader = nullptr;

	// Inherited via QuitListener
	virtual void OnQuit() override;

	// Inherited via WindowListener
	virtual void OnResize(int width, int height) override;
};