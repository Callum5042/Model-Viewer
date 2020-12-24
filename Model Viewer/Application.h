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

	// Camera settings
	float m_Pitch = 30.0f;
	float m_Yaw = 0.0f;
	int m_CameraRotationSpeed = 20;
	std::pair<int, int> m_LastMousePosition;

	// Query system info
	void QueryHardwareInfo();
	std::string m_CpuName;
	std::string m_GpuName;
	std::string m_RamAmount;
	std::string m_VideoRamAmount;

	// Anti-aliasing
	std::string m_CurrentAntiAliasingLevel;
	std::vector<std::string> m_AntiAliasingLevelsText;

	// Inherited via QuitListener
	virtual void OnQuit() override;

	// Inherited via WindowListener
	virtual void OnResize(int width, int height) override;

	// Inherited via KeyboardListener
	virtual void OnKeyPressed(SDL_Scancode scancode) override;

	// Inherited via MouseListener
	virtual void OnMouseMove(const MouseData& mouse) override;
	virtual void OnMousePressed(const MouseData& mouse) override;
	virtual void OnMouseReleased(const MouseData& mouse) override;
};