#include "Pch.h"
#include "Application.h"
#include "Gui.h"
#include "Shader.h"

Application::Application()
{
	m_Window = std::make_unique<Window>();
	m_EventDispatcher = std::make_unique<EventDispatcher>();

	m_Renderer = std::make_unique<DxRenderer>();
	m_Shader = std::make_unique<DxShader>(m_Renderer.get());
	m_Camera = std::make_unique<Camera>(800, 600);
	m_Model = std::make_unique<DxModel>(m_Renderer.get());
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
		std::string fps = "FPS: " + std::to_string(m_FramesPerSecond);
		ImGui::Text(fps.c_str());

		// Display CPU name
		ImGui::Text(m_CpuName.c_str());

		// Display device name
		ImGui::Text(m_GpuName.c_str());

		// Display system RAM
		ImGui::Text(m_RamAmount.c_str());

		// Display video RAM
		ImGui::Text(m_VideoRamAmount.c_str());
	}

	ImGui::End();

	// Button
	static bool rendererOpen = true;
	ImGui::Begin("Renderer", &rendererOpen);

	if (m_Renderer->GetRenderAPI() == RenderAPI::DIRECTX)
	{
		ImGui::Text("DirectX");
	}
	else if (m_Renderer->GetRenderAPI() == RenderAPI::OPENGL)
	{
		ImGui::Text("OpenGL");
	}

	if (ImGui::Button("OpenGL"))
	{
		m_SwitchRenderAPI = RenderAPI::OPENGL;
	}
	else if (ImGui::Button("DirectX"))
	{
		m_SwitchRenderAPI = RenderAPI::DIRECTX;
	}

	if (ImGui::Checkbox("Wireframe (Press 1)", &m_Wireframe))
	{
		m_Renderer->ToggleWireframe(m_Wireframe);
	}

	ImGui::End();
	Gui::Render(m_Renderer.get());
}

void Application::ChangeRenderAPI()
{
	if (m_SwitchRenderAPI != RenderAPI::NONE)
	{
		Gui::Destroy(m_Renderer.get());

		m_Window.reset();
		m_Renderer.reset();
		m_Model.reset();
		m_Shader.reset();
		m_Camera.reset();

		if (m_SwitchRenderAPI == RenderAPI::DIRECTX)
		{
			m_Window = std::make_unique<Window>();
			m_Renderer = std::make_unique<DxRenderer>();
			m_Shader = std::make_unique<DxShader>(m_Renderer.get());
			m_Camera = std::make_unique<Camera>(800, 600);
			m_Model = std::make_unique<DxModel>(m_Renderer.get());
		}
		else if (m_SwitchRenderAPI == RenderAPI::OPENGL)
		{
			m_Window = std::make_unique<OpenGLWindow>();
			m_Renderer = std::make_unique<GlRenderer>();
			m_Shader = std::make_unique<GlShader>();
			m_Camera = std::make_unique<GlCamera>(800, 600);
			m_Model = std::make_unique<GlModel>(m_Shader.get());
		}

		Init();
		m_SwitchRenderAPI = RenderAPI::NONE;
	}
}

void Application::QueryHardwareInfo()
{
	std::array<int, 4> cpui;
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

void Application::OnMouseMove(MouseData&& mouse)
{
	if (mouse.state == SDL_BUTTON_LMASK)
	{

	}
}
