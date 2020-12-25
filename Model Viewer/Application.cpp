#include "Pch.h"
#include "Application.h"
#include "Gui.h"
#include "Shader.h"

Application::Application()
{
	m_EventDispatcher = std::make_unique<EventDispatcher>();

	auto startup = RenderAPI::OPENGL;
	if (startup == RenderAPI::DIRECTX)
	{
		m_Window = std::make_unique<Window>();
		m_Renderer = std::make_unique<DxRenderer>();
		m_Shader = std::make_unique<DxShader>(m_Renderer.get());
		m_Camera = std::make_unique<Camera>(800, 600, m_Fov);
		m_Model = std::make_unique<DxModel>(m_Renderer.get());
	}
	else if (startup == RenderAPI::OPENGL)
	{
		m_Window = std::make_unique<OpenGLWindow>();
		m_Renderer = std::make_unique<GlRenderer>();
		m_Shader = std::make_unique<GlShader>();
		m_Camera = std::make_unique<GlCamera>(800, 600, m_Fov);
		m_Model = std::make_unique<GlModel>(m_Shader.get());
	}
}

Application::~Application()
{
	Gui::Destroy(m_Renderer.get());
	SDL_Quit();
}

int Application::Execute()
{
	// Initialise SDL
	if (SDL_Init(SDL_INIT_EVENTS) != 0)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "SDL_Init failed!", nullptr);
		return -1;
	}

	if (!Init())
	{
		return -1;
	}

	m_EventDispatcher->Attach(static_cast<QuitListener*>(this));
	m_EventDispatcher->Attach(static_cast<WindowListener*>(this));
	m_EventDispatcher->Attach(static_cast<KeyboardListener*>(this));
	m_EventDispatcher->Attach(static_cast<MouseListener*>(this));

	RenderAPI switchApi = RenderAPI::NONE;

	m_Timer.Start();
	while (m_Running)
	{
		m_Timer.Tick();
		CalculateFramesPerSecond();

		m_EventDispatcher->Poll();
		Render();

		ChangeRenderAPI();
	}

	return 0;
}

void Application::CalculateFramesPerSecond()
{
	static double time = 0;
	static int frameCount = 0;

	frameCount++;
	time += m_Timer.DeltaTime();
	if (time > 1.0f)
	{
		m_FramesPerSecond = frameCount;
		time = 0.0f;
		frameCount = 0;
	}
}

bool Application::Init()
{
	// Create window
	if (!m_Window->Create("Model Viewer", 800, 600, WindowMode::WINDOW))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Window::Create failed!", nullptr);
		return false;
	}

	// Create renderer
	if (!m_Renderer->Create(m_Window.get()))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Renderer::Create failed!", nullptr);
		return false;
	}

	m_Renderer->ToggleWireframe(m_Wireframe);

	// Setup ImGui
	if (!Gui::Init(m_Window.get(), m_Renderer.get()))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Gui::Init failed!", nullptr);
		return false;
	}

	// Create shaders
	if (!m_Shader->Create())
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "IShader::Create failed!", nullptr);
		return false;
	}

	// Load model
	if (!m_Model->Load())
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "IModel::Load failed!", nullptr);
		return false;
	}

	QueryHardwareInfo();
	m_Camera->SetPitchAndYaw(m_Pitch, m_Yaw);

	// Check MSAA levels
	m_AntiAliasingLevelsText.clear();
	for (auto& x : m_Renderer->GetSupportMsaaLevels())
	{
		auto str = "x" + std::to_string(x);
		m_AntiAliasingLevelsText.push_back(str);
	}

	m_AntiAliasingLevelsText.push_back("Off");
	std::reverse(m_AntiAliasingLevelsText.begin(), m_AntiAliasingLevelsText.end());

	// Get current MSAA level
	auto level = m_Renderer->GetCurrentMsaaLevel();
	level = 0;
	if (level == 0)
	{
		m_CurrentAntiAliasingLevel = "Off";
	}
	else
	{
		m_CurrentAntiAliasingLevel = "x" + std::to_string(level);
	}

	m_Renderer->CreateAntiAliasingTarget(level, m_Window->GetWidth(), m_Window->GetHeight());

	return true;
}

void Application::Render()
{
	m_Renderer->Clear();
	m_Shader->Use();
	m_Model->Render(m_Camera.get());
	RenderGui();
	m_Renderer->Present();
}

void Application::RenderGui()
{
	// Gui
	Gui::StartFrame(m_Window.get(), m_Renderer.get());

	//bool show_demo_window = true;
	//ImGui::ShowDemoWindow(&show_demo_window);

	// Info
	const float distance = 15.0f;
	bool open = true;
	ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
	ImGui::SetNextWindowPos(ImVec2(0 + distance, 0 + distance), ImGuiCond_Always);
	if (ImGui::Begin("FPS Display", &open, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
	{
		// Display FPS
		std::string fps = "FPS: " + std::to_string(m_FramesPerSecond) + " - " + std::to_string(1000.0f / m_FramesPerSecond) + " ms";
		ImGui::Text(fps.c_str());

		// Display CPU name
		ImGui::Text(m_CpuName.c_str());

		// Display device name
		ImGui::Text(m_GpuName.c_str());

		// Display system RAM
		ImGui::Text(m_RamAmount.c_str());

		// Display video RAM
		ImGui::Text(m_VideoRamAmount.c_str());

		// Camera
		auto pitch = "Pitch: " + std::to_string(m_Pitch);
		ImGui::Text(pitch.c_str());

		auto yaw = "Yaw: " + std::to_string(m_Yaw);
		ImGui::Text(yaw.c_str());

		auto fov = "Field of view: " + std::to_string(static_cast<int>(m_Fov));
		ImGui::Text(fov.c_str());
	}

	ImGui::End();

	// Options
	{
		static bool rendererOpen = true;
		ImGui::Begin("Renderer", &rendererOpen);

		// Display rendering API
		static int current_combo_render_api = static_cast<int>(m_Renderer->GetRenderAPI()) - 1;
		const char* combo_render_api_items[] = { "DirectX", "OpenGL" };
		if (ImGui::Combo("##RenderAPI", &current_combo_render_api, combo_render_api_items, IM_ARRAYSIZE(combo_render_api_items)))
		{
			m_SwitchRenderAPI = static_cast<RenderAPI>(current_combo_render_api + 1);
		}

		// Anti-aliasing
		if (ImGui::BeginCombo("##AntiAliasing", m_CurrentAntiAliasingLevel.c_str(), 0))
		{
			for (auto& x : m_AntiAliasingLevelsText)
			{
				bool selected = (m_CurrentAntiAliasingLevel == x.c_str());
				if (ImGui::Selectable(x.c_str(), &selected))
				{
					m_CurrentAntiAliasingLevel = x.c_str();

					int level = 0;
					if (x != "Off")
					{
						auto substr = x.substr(1, x.size() - 1);
						level = std::stoi(substr);
					}

					m_Renderer->CreateAntiAliasingTarget(level, m_Window->GetWidth(), m_Window->GetHeight());
				}
			}

			ImGui::EndCombo();
		}

		// Wireframe
		if (ImGui::Checkbox("Wireframe (Press 1)", &m_Wireframe))
		{
			m_Renderer->ToggleWireframe(m_Wireframe);
		}

		// Camera
		ImGui::SliderInt("Camera Speed", &m_CameraRotationSpeed, 1, 100);

		ImGui::End();
	}

	Gui::Render(m_Renderer.get());
}

void Application::ChangeRenderAPI()
{
	if (m_SwitchRenderAPI != RenderAPI::NONE)
	{
		Gui::Destroy(m_Renderer.get());

		// Get window state
		std::pair<int, int> window_position;
		m_Window->GetPosition(&window_position.first, &window_position.second);

		auto window_width = m_Window->GetWidth();
		auto window_height = m_Window->GetHeight();
		auto maximised = m_Window->IsMaximised();

		// Release memory
		m_Window.reset();
		m_Renderer.reset();
		m_Model.reset();
		m_Shader.reset();
		m_Camera.reset();

		// Switch Rendering API
		if (m_SwitchRenderAPI == RenderAPI::DIRECTX)
		{
			m_Window = std::make_unique<Window>();
			m_Renderer = std::make_unique<DxRenderer>();
			m_Shader = std::make_unique<DxShader>(m_Renderer.get());
			m_Camera = std::make_unique<Camera>(window_width, window_height, m_Fov);
			m_Model = std::make_unique<DxModel>(m_Renderer.get());
		}
		else if (m_SwitchRenderAPI == RenderAPI::OPENGL)
		{
			m_Window = std::make_unique<OpenGLWindow>();
			m_Renderer = std::make_unique<GlRenderer>();
			m_Shader = std::make_unique<GlShader>();
			m_Camera = std::make_unique<GlCamera>(window_width, window_height, m_Fov);
			m_Model = std::make_unique<GlModel>(m_Shader.get());
		}

		Init();

		// Restore window state
		m_Window->SetPosition(window_position.first, window_position.second);
		m_Renderer->Resize(window_width, window_height);

		if (maximised)
		{
			m_Window->Maximise();
		}
		else
		{
			m_Window->SetWidth(window_width).SetHeight(window_height);
		}

		// Reset switch flag
		m_SwitchRenderAPI = RenderAPI::NONE;
	}
}

void Application::QueryHardwareInfo()
{
	std::array<int, 4> cpui{};
	__cpuid(cpui.data(), 0);
	auto nIds_ = cpui[0];
	auto nExIds_ = cpui[0];

	std::bitset<32> f_1_ECX_;
	std::bitset<32> f_1_EDX_;
	std::bitset<32> f_7_EBX_;
	std::bitset<32> f_7_ECX_;
	std::bitset<32> f_81_ECX_;
	std::bitset<32> f_81_EDX_;

	std::vector<std::array<int, 4>> data_;
	std::vector<std::array<int, 4>> extdata_;
	for (int i = 0; i <= nIds_; ++i)
	{
		__cpuidex(cpui.data(), i, 0);
		data_.push_back(cpui);
	}

	// Vendor
	char vendor[0x20];
	memset(vendor, 0, sizeof(vendor));
	*reinterpret_cast<int*>(vendor) = data_[0][1];
	*reinterpret_cast<int*>(vendor + 4) = data_[0][3];
	*reinterpret_cast<int*>(vendor + 8) = data_[0][2];

	// load bitset with flags for function 0x00000001
	if (nIds_ >= 1)
	{
		f_1_ECX_ = data_[1][2];
		f_1_EDX_ = data_[1][3];
	}

	// load bitset with flags for function 0x00000007
	if (nIds_ >= 7)
	{
		f_7_EBX_ = data_[7][1];
		f_7_ECX_ = data_[7][2];
	}

	// Calling __cpuid with 0x80000000 as the function_id argument
	// gets the number of the highest valid extended ID.
	__cpuid(cpui.data(), 0x80000000);
	nExIds_ = cpui[0];

	char brand[0x40];
	memset(brand, 0, sizeof(brand));

	for (int i = 0x80000000; i <= nExIds_; ++i)
	{
		__cpuidex(cpui.data(), i, 0);
		extdata_.push_back(cpui);
	}

	// load bitset with flags for function 0x80000001
	if (nExIds_ >= 0x80000001)
	{
		f_81_ECX_ = extdata_[1][2];
		f_81_EDX_ = extdata_[1][3];
	}

	// CPU brand
	memset(brand, 0, sizeof(brand));
	if (nExIds_ >= 0x80000004)
	{
		memcpy(brand, extdata_[2].data(), sizeof(cpui));
		memcpy(brand + 16, extdata_[3].data(), sizeof(cpui));
		memcpy(brand + 32, extdata_[4].data(), sizeof(cpui));
		m_CpuName = brand;
	}

	// Display device name
	m_GpuName = m_Renderer->GetName();

	// Display system RAM
	m_RamAmount = "RAM: " + std::to_string(SDL_GetSystemRAM()) + "MB";

	// Display video RAM
	m_VideoRamAmount = "VRAM: " + std::to_string(m_Renderer->GetVRAM() / 1024 / 1024) + "MB";
}

void Application::OnQuit()
{
	m_Running = false;
}

void Application::OnResize(int width, int height)
{
	m_Renderer->Resize(width, height);
	m_Camera->Resize(width, height);
}

#pragma warning(push)
#pragma warning(disable : 26812)
void Application::OnKeyPressed(SDL_Scancode scancode)
{
#pragma warning(pop)
	if (scancode == SDL_SCANCODE_1)
	{
		m_Wireframe = !m_Wireframe;
		m_Renderer->ToggleWireframe(m_Wireframe);
	}
}

void Application::OnMouseMove(const MouseData& mouse)
{
	if (mouse.state == SDL_BUTTON_LMASK)
	{
		float dt = static_cast<float>(m_Timer.DeltaTime());

		m_Yaw += (static_cast<float>(mouse.xrel) * m_CameraRotationSpeed / 100);// *dt * m_CameraRotationSpeed * 100);
		m_Pitch += (static_cast<float>(mouse.yrel) * m_CameraRotationSpeed / 100);// *dt * m_CameraRotationSpeed * 100);

		m_Yaw = (m_Yaw > 360.0f ? 0.0f : m_Yaw);
		m_Yaw = (m_Yaw < 0.0f ? 360.0f : m_Yaw);
		m_Pitch = std::clamp<float>(m_Pitch, -89, 89);

		m_Camera->SetPitchAndYaw(m_Pitch, m_Yaw);
	}
}

void Application::OnMousePressed(const MouseData& mouse)
{
}

void Application::OnMouseReleased(const MouseData& mouse)
{
}

void Application::OnMouseWheel(const MouseData& mouse)
{
	m_Fov -= static_cast<int>(mouse.y);
	m_Fov = std::clamp<float>(m_Fov, 1.0f, 180.0f);
	m_Camera->SetFov(m_Fov);
}
