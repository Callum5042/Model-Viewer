#pragma once

#define GLEW_STATIC
#include <gl/glew.h>
#include <gl/wglew.h>

#ifdef _WIN32
	#include <d3d11_4.h>
	#include <wrl\client.h>
	using Microsoft::WRL::ComPtr;
#endif

#include <string>
#include <vector>
#include <iostream>
#include <memory>
#include <sstream>
#include <locale>
#include <codecvt>        // std::codecvt_utf8
#include <cstdint>
#include <array>
#include <bitset>

#include <SDL.h>
#include <SDL_syswm.h>