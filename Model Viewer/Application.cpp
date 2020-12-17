#include "Pch.h"
#include "Application.h"

Application::Application()
{
    m_Window = std::make_unique<Window>();
    m_EventDispatcher = std::make_unique<EventDispatcher>();
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
    m_EventDispatcher->Attach(m_Window.get());
    if (!m_Window->Create("Test", 800, 600))
    {
    	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Window::Create failed!", nullptr);
    	return false;
    }

    m_EventDispatcher->Attach(this);
    return true;
}

void Application::OnQuit()
{
    m_Running = false;
}
