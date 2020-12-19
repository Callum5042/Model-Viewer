#include "Pch.h"
#include "Application.h"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_dx11.h"

Application::Application()
{
    m_Window = std::make_unique<Window>();
    m_EventDispatcher = std::make_unique<EventDispatcher>();
    m_Renderer = std::make_unique<Renderer>();
}

Application::~Application()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_Quit();
}

int Application::Execute()
{
    if (!Init())
    {
        return -1;
    }

    // ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::GetStyle().WindowRounding = 0.0f;

    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL2_InitForD3D(m_Window->GetSdlWindow()))
        return -1;

    if (!ImGui_ImplDX11_Init(m_Renderer->GetDevice().Get(), m_Renderer->GetDeviceContext().Get()))
        return -1;

    m_Timer.Start();
    while (m_Running)
    {
        m_Timer.Tick();
        CalculateFramesPerSecond();

        m_EventDispatcher->Poll();

        // Gui
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplSDL2_NewFrame(m_Window->GetSdlWindow());
        ImGui::NewFrame();

        bool show_demo_window = true;
        ImGui::ShowDemoWindow(&show_demo_window);

        const float distance = 15.0f;
        bool open = true;
        ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
        ImGui::SetNextWindowPos(ImVec2(0 + distance, 0 + distance), ImGuiCond_Always);
        if (ImGui::Begin("FPS Display", &open, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
        {
            std::string fps = "FPS: " + std::to_string(m_FramesPerSecond);
            ImGui::Text(fps.c_str());
            ImGui::Text(m_Renderer->GetDescription().c_str());

            /*const char* items[] = { "DirectX 11", "OpenGL" };
            static int item_current = 0;
            ImGui::Combo("##renderingApi", &item_current, items, IM_ARRAYSIZE(items));*/
        }

        ImGui::End();

        ImGui::Render();

        m_Renderer->Clear();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        m_Renderer->Present();
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
    // Initialise SDL
    if (SDL_Init(SDL_INIT_EVENTS) != 0)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "SDL_Init failed!", nullptr);
        return false;
    }

    // Create window
    if (!m_Window->Create("Model Viewer", 800, 600))
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

    m_EventDispatcher->Attach(static_cast<QuitListener*>(this));
    m_EventDispatcher->Attach(static_cast<WindowListener*>(this));
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
