#include "Pch.h"
#include "Application.h"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_opengl3.h"

Application::Application()
{
    m_Window = std::make_unique<Window>();
    m_EventDispatcher = std::make_unique<EventDispatcher>();
    m_Renderer = std::make_unique<DxRenderer>();
}

Application::~Application()
{
    ImGui_ImplSDL2_Shutdown();

    if (m_Renderer->GetRenderAPI() == RenderAPI::DIRECTX)
    {
        ImGui_ImplDX11_Shutdown();
    }
    else if (m_Renderer->GetRenderAPI() == RenderAPI::OPENGL)
    {
        ImGui_ImplOpenGL3_Shutdown();
    }

    ImGui::DestroyContext();

    SDL_Quit();
}

int Application::Execute()
{
    // Initialise SDL
    if (SDL_Init(SDL_INIT_EVENTS) != 0)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "SDL_Init failed!", nullptr);
        return false;
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

        // Gui
        if (m_Renderer->GetRenderAPI() == RenderAPI::DIRECTX)
        {
            ImGui_ImplDX11_NewFrame();
        }
        else if (m_Renderer->GetRenderAPI() == RenderAPI::OPENGL)
        {
            ImGui_ImplOpenGL3_NewFrame();
        }

        ImGui_ImplSDL2_NewFrame(m_Window->GetSdlWindow());
        ImGui::NewFrame();

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
            switchApi = RenderAPI::OPENGL;
        }
        if (ImGui::Button("DirectX"))
        {
            switchApi = RenderAPI::DIRECTX;
        }

        ImGui::End();

        ImGui::Render();

        m_Renderer->Clear();

        if (m_Renderer->GetRenderAPI() == RenderAPI::DIRECTX)
        {
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        }
        else if (m_Renderer->GetRenderAPI() == RenderAPI::OPENGL)
        {
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        m_Renderer->Present();

        if (switchApi != RenderAPI::NONE)
        {
            if (m_Renderer->GetRenderAPI() == RenderAPI::DIRECTX)
            {
                ImGui_ImplDX11_Shutdown();
            }
            else if (m_Renderer->GetRenderAPI() == RenderAPI::OPENGL)
            {
                ImGui_ImplOpenGL3_Shutdown();
            }

            ImGui_ImplSDL2_Shutdown();
            ImGui::DestroyContext();

            m_Window.reset();
            m_Renderer.reset();

            if (switchApi == RenderAPI::DIRECTX)
            {
                m_Window = std::make_unique<Window>();
                m_Renderer = std::make_unique<DxRenderer>();
            }
            else if (switchApi == RenderAPI::OPENGL)
            {
                m_Window = std::make_unique<OpenGLWindow>();
                m_Renderer = std::make_unique<GlRenderer>();
            }

            Init();
            switchApi = RenderAPI::NONE;
        }
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
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::GetStyle().WindowRounding = 0.0f;
    ImGui::StyleColorsDark();

    if (m_Renderer->GetRenderAPI() == RenderAPI::DIRECTX)
    {
        if (!ImGui_ImplSDL2_InitForD3D(m_Window->GetSdlWindow()))
            return false;

        DxRenderer* renderer = reinterpret_cast<DxRenderer*>(m_Renderer.get());
        if (!ImGui_ImplDX11_Init(renderer->GetDevice().Get(), renderer->GetDeviceContext().Get()))
            return false;
    }
    else if (m_Renderer->GetRenderAPI() == RenderAPI::OPENGL)
    {
        OpenGLWindow* glWindow = reinterpret_cast<OpenGLWindow*>(m_Window.get());

        if (!ImGui_ImplSDL2_InitForOpenGL(glWindow->GetSdlWindow(), glWindow->GetOpenGLContext()))
            return false;

        GlRenderer* renderer = reinterpret_cast<GlRenderer*>(m_Renderer.get());
        if (!ImGui_ImplOpenGL3_Init())
            return false;
    }

    return true;
}

void Application::OnQuit()
{
    m_Running = false;
}

void Application::OnResize(int width, int height)
{
    m_Renderer->Resize(width, height);
}
