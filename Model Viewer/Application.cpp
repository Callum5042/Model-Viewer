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
        std::string fps = "FPS: " + std::to_string(m_FramesPerSecond);
        ImGui::Text(fps.c_str());
        ImGui::Text(m_Renderer->GetDescription().c_str());

        int ram = SDL_GetSystemRAM();
        std::string ramStr = "RAM: " + std::to_string(ram) + "MB";
        ImGui::Text(ramStr.c_str());
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

void Application::OnQuit()
{
    m_Running = false;
}

void Application::OnResize(int width, int height)
{
    m_Renderer->Resize(width, height);
}
