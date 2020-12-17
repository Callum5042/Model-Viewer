#include "Pch.h"
#include "Application.h"

Application::Application()
{
    m_Window = std::make_unique<Window>();
    m_EventDispatcher = std::make_unique<EventDispatcher>();
    m_Renderer = std::make_unique<Renderer>();
}

Application::~Application()
{
}

int Application::Execute()
{
    if (!Init())
    {
        return -1;
    }

    while (m_Running)
    {
        m_EventDispatcher->Poll();

        m_Renderer->Clear();
        m_Renderer->Present();
    }

    return 0;
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
    if (!m_Window->Create("Test", 800, 600))
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

void Application::OnResize()
{
}
