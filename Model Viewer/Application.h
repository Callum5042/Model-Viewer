#pragma once

#include "EventDispatcher.h"
#include "Timer.h"
#include "Shader.h"
#include "Camera.h"

// Forward declarions
class Window;
class IRenderer;
class IModel;
class ICamera;
class IShader;

// Core application
class Application final : public QuitListener, public WindowListener, public KeyboardListener, public MouseListener
{
public:
	Application();
	virtual ~Application();
	Application& operator=(const Application&) = delete;
	Application(const Application&) = delete;

	// Application entry point. Returns 0 on exit success
	int Execute();

private:
	bool m_Running = true;
	Timer m_Timer;

	// Core functions
	bool Init();
	void Update(float dt);
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
	std::unique_ptr<IModel> m_Model = nullptr;


	std::unique_ptr<Camera> m_DxCamera = nullptr;

	// Rendering API switch
	void ChangeRenderAPI();
	RenderAPI m_SwitchRenderAPI = RenderAPI::NONE;

	// Wireframe state
	bool m_Wireframe = false;

	// Camera settings
	float m_Pitch = 30.0f;
	float m_Yaw = 0.0f;
	float m_Fov = 50.0f;
	float m_Radius = -8.0f;
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

	// Texture filtering GUI
	std::string m_CurrentTextureFilterLevel;
	std::vector<std::string> m_TextureFilteringLevelsText;

	// Vsync
	bool m_Vsync = false;

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
	virtual void OnMouseWheel(const MouseData& mouse) override;

	// Model path
	std::string m_ModelPath;
};