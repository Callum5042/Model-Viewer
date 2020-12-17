#include "Pch.h"
#include "Window.h"

Window::~Window()
{
    Destroy();
}

bool Window::Create(std::string&& title, int width, int height)
{
    m_Window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
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
    int width = 0;
    SDL_GetWindowSize(m_Window, &width, nullptr);
    return width;
}

int Window::GetHeight() const
{
    int height = 0;
    SDL_GetWindowSize(m_Window, nullptr, &height);
    return height;
}
