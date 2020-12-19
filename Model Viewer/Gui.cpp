#include "Pch.h"
#include "Gui.h"
#include "Window.h"
#include "Renderer.h"

#include "imgui_impl_sdl.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_opengl3.h"

bool Gui::Init(Window* window, IRenderer* renderer)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::GetStyle().WindowRounding = 0.0f;
    ImGui::StyleColorsDark();

    if (renderer->GetRenderAPI() == RenderAPI::DIRECTX)
    {
        if (!ImGui_ImplSDL2_InitForD3D(window->GetSdlWindow()))
            return false;

        DxRenderer* dxRenderer = reinterpret_cast<DxRenderer*>(renderer);
        if (!ImGui_ImplDX11_Init(dxRenderer->GetDevice().Get(), dxRenderer->GetDeviceContext().Get()))
            return false;
    }
    else if (renderer->GetRenderAPI() == RenderAPI::OPENGL)
    {
        OpenGLWindow* glWindow = reinterpret_cast<OpenGLWindow*>(window);

        if (!ImGui_ImplSDL2_InitForOpenGL(glWindow->GetSdlWindow(), glWindow->GetOpenGLContext()))
            return false;

        GlRenderer* glRenderer = reinterpret_cast<GlRenderer*>(renderer);
        if (!ImGui_ImplOpenGL3_Init())
            return false;
    }

    return true;
}

void Gui::Destroy(IRenderer* renderer)
{
    ImGui_ImplSDL2_Shutdown();

    switch (renderer->GetRenderAPI())
    {
    case RenderAPI::DIRECTX:
        ImGui_ImplDX11_Shutdown();
        break;

    case RenderAPI::OPENGL:
        ImGui_ImplOpenGL3_Shutdown();
        break;
    };

    ImGui::DestroyContext();
}

void Gui::StartFrame(Window* window, IRenderer* renderer)
{
    switch (renderer->GetRenderAPI())
    {
    case RenderAPI::DIRECTX:
        ImGui_ImplDX11_NewFrame();
        break;

    case RenderAPI::OPENGL:
        ImGui_ImplOpenGL3_NewFrame();
        break;
    };

    ImGui_ImplSDL2_NewFrame(window->GetSdlWindow());
    ImGui::NewFrame();
}

void Gui::Render(IRenderer* renderer)
{
    ImGui::Render();
    switch (renderer->GetRenderAPI())
    {
    case RenderAPI::DIRECTX:
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        break;

    case RenderAPI::OPENGL:
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        break;
    };
}
