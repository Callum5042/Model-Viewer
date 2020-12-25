#pragma once

/*
	MIT License

	Copyright (c) 2020 Callum Anning

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.

	// References
	https://www.khronos.org/opengl/wiki/S3_Texture_Compression
	https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds-pguide

*/

#include <string>
#include <vector>
#include <filesystem>
typedef unsigned int uint;
typedef unsigned char uchar;

namespace Rove
{
	struct DDSMipmap
	{
		int width = 0;
		int height = 0;
		uchar* data = nullptr;

		int texture_size = 0;
		int level = 0;
	};

	class LoadDDS
	{
	public:
		LoadDDS() = default;
		LoadDDS(std::filesystem::path path);
		LoadDDS(std::string&& path);
		virtual ~LoadDDS();

		void Load(std::string&& path);
		void Load(std::filesystem::path path);

		constexpr bool IsLoaded() { return m_Success; }
		constexpr std::string& GetError() { return m_ErrorMessage; }

		constexpr int Width() { return m_Width; }
		constexpr int Height() { return m_Height; }
		constexpr uint Format() { return m_Format; }

		constexpr int MipmapCount() { return m_MipmapCount; }

		std::vector<DDSMipmap> mipmaps;

	private:
		bool m_Success = false;
		std::string m_ErrorMessage;

		int m_Width = 0;
		int m_Height = 0;
		int m_MipmapCount = 0;

		uint m_Format = 0;
	};
}