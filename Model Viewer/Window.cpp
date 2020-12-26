#include "Pch.h"
#include "Window.h"

Window::~Window()
{
    Destroy();
}

bool Window::Create(std::string&& title, int width, int height, WindowMode windowMode)
{
    auto windowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_MAXIMIZED;
    if (windowMode == WindowMode::BORDERLESS_FULLSCREEN)
    {
        windowFlags |= SDL_WINDOW_MAXIMIZED | SDL_WINDOW_BORDERLESS;
    }

    m_Window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, windowFlags);
    if (m_Window == nullptr)
    {
        return false;
    }

    return true;
}

void Window::Destroy()
{
    SDL_DestroyWindow(m_Window);
}

int Window::GetWidth() const
{
    auto width = 0;
    SDL_GetWindowSize(m_Window, &width, nullptr);
    return width;
}

int Window::GetHeight() const
{
    auto height = 0;
    SDL_GetWindowSize(m_Window, nullptr, &height);
    return height;
}

Window& Window::SetWidth(int width)
{
    SDL_SetWindowSize(m_Window, width, GetHeight());
    return *this;
}

Window& Window::SetHeight(int height)
{
    SDL_SetWindowSize(m_Window, GetWidth(), height);
    return *this;
}

void Window::SetPosition(int x, int y)
{
    SDL_SetWindowPosition(m_Window, x, y);
}

void Window::GetPosition(int* x, int* y)
{
    SDL_GetWindowPosition(m_Window, x, y);
}

bool Window::IsMaximised() const
{
    auto flags = SDL_GetWindowFlags(m_Window);
    if (flags & SDL_WINDOW_MAXIMIZED)
    {
        return true;
    }

    return false;
}

void Window::Maximise()
{
    SDL_MaximizeWindow(m_Window);
}

OpenGLWindow::~OpenGLWindow()
{
    SDL_GL_DeleteContext(m_OpenGlContext);
    Destroy();
}

bool OpenGLWindow::Create(std::string&& title, int width, int height, WindowMode windowMode)
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

    //SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    //SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    auto windowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL;
    if (windowMode == WindowMode::BORDERLESS_FULLSCREEN)
    {
        windowFlags |= SDL_WINDOW_MAXIMIZED | SDL_WINDOW_BORDERLESS;
    }

    m_Window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, windowFlags);
    if (m_Window == nullptr)
    {
        return false;
    }

    // OpenGL Context
    m_OpenGlContext = SDL_GL_CreateContext(m_Window);
    if (m_OpenGlContext == nullptr)
    {
        SDL_ShowSimpleMessageBox(NULL, "Error", "SDL_GL_CreateContext failed!", nullptr);
        return false;
    }

    return true;
}
