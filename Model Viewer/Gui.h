#pragma once

#include "imgui.h"

class Window;
class IRenderer;

namespace Gui
{
	bool Init(Window* window, IRenderer* renderer);

	void Destroy(IRenderer* renderer);

	void StartFrame(Window* window, IRenderer* renderer);

	void Render(IRenderer* renderer);
}