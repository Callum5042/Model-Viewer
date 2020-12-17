#include "Pch.h"
#include "Window.h"

bool Window::Create(std::string&& title, int width, int height)
{
    m_Window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (m_Window == nullptr)
    {
        return false;
    }

    return true;
}

void Window::OnResize()
{
    std::cout << "Resize\n";
}
