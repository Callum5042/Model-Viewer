#pragma once

#include "EventDispatcher.h"
#include "Window.h"
#include "Renderer.h"
#include "Timer.h"
#include "Model.h"
#include "Camera.h"
#include "Shader.h"

class Application : public QuitListener, public WindowListener, public KeyboardListener, public MouseListener
{
public:
	Application();
	virtual ~Application();

	int Execute();

private:
	bool m_Running = true;
	Timer m_Timer;

	// Core functions
	bool Init();
	void Render();

	// Dear ImGui rendering function
	void RenderGui();

	// Calculate FPS
	int m_FramesPerSecond = 0;
	void CalculateFramesPerSecond();

	// Window
	std::unique_ptr<Window> m_Window = nullptr;

	// Event system
	std::unique_ptr<EventDispatcher> m_EventDispatcher = nullptr;

	// Rendering pipeline
	std::unique_ptr<IRenderer> m_Renderer = nullptr;
	std::unique_ptr<IShader> m_Shader = nullptr;
	std::unique_ptr<ICamera> m_Camera = nullptr;
	std::unique_ptr<IModel> m_Model = nullptr;

	// Rendering API switch
	void ChangeRenderAPI();
	RenderAPI m_SwitchRenderAPI = RenderAPI::NONE;

	// Wireframe state
	bool m_Wireframe = false;

	// Query system info
	void QueryHardwareInfo();
	std::string m_CpuName;
	std::string m_GpuName;
	std::string m_RamAmount;
	std::string m_VideoRamAmount;

	// Inherited via QuitListener
	virtual void OnQuit() override;

	// Inherited via WindowListener
	virtual void OnResize(int width, int height) override;

	// Inherited via KeyboardListener
	virtual void OnKeyPressed(SDL_Scancode scancode) override;

	// Inherited via MouseListener
	virtual void OnMouseMove(MouseData&& mouse) override;
};