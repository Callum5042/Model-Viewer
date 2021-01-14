#pragma once

#define GLEW_STATIC
#include <gl/glew.h>
#include <gl/wglew.h>

#ifdef _WIN32
	#include <d3d11_4.h>
	#include <wrl\client.h>
	using Microsoft::WRL::ComPtr;
#endif

// dunno if we want this tbh - not currently used
#ifndef NOEXCEPT
#ifdef _DEBUG
	#define NOEXCEPT noexcept
#else
#define NOTHROW
	#endif
#endif

#include <string>
#include <vector>
#include <iostream>
#include <memory>
#include <sstream>
#include <locale>
#include <codecvt> // std::codecvt_utf8
#include <cstdint>
#include <array>
#include <bitset>
#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <chrono>
#include <optional>

#include <SDL.h>
#include <SDL_syswm.h>

#undef min
#undef max