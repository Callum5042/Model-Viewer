#include "Pch.h"
#include "LoadTextureDDS.h"
#pragma warning(disable : 6385)

Rove::LoadDDS::LoadDDS(std::filesystem::path path)
{
	Load(path);
}

Rove::LoadDDS::LoadDDS(const std::string& path)
{
	Load(path);
}

Rove::LoadDDS::~LoadDDS()
{
	for (auto& x : mipmaps)
	{
		delete x.data;
	}

	mipmaps.clear();
}

void Rove::LoadDDS::Load(const std::string& path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		m_ErrorMessage = "Could not open file: " + path;
		return;
	}

	std::streampos begin = file.tellg();
	file.seekg(0, std::ios::end);
	std::streampos end = file.tellg();

	auto size = static_cast<int>(end - begin);
	auto buffer = new char[size];

	file.seekg(0, std::ios::beg);
	file.read(buffer, size);
	file.close();

	// Check that it has DDS header
	if (buffer[0] != 'D' || buffer[1] != 'D' || buffer[2] != 'S')
	{
		m_ErrorMessage = "Not a DDS file";
		return;
	}

	// Get header data
	m_Height = (buffer[12]) | (buffer[13] << 8) | (buffer[14] << 16) | (buffer[15] << 24);
	m_Width = (buffer[16]) | (buffer[17] << 8) | (buffer[18] << 16) | (buffer[19] << 24);
	m_MipmapCount = (buffer[28]) | (buffer[29] << 8) | (buffer[30] << 16) | (buffer[31] << 24);

	// Get format
	int blockSize = 0;
	switch (buffer[87])
	{
	case '1':
		m_Format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		blockSize = 8;
		break;

	case '3':
		m_Format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		blockSize = 16;
		break;

	case '5':
		m_Format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		blockSize = 16;
		break;

	default:
		m_ErrorMessage = "Not supported";
		return;
	}

	// Load data into mipmap vector
	const int header_size = 128;

	auto width = m_Width;
	auto height = m_Height;

	auto offset = 0;
	for (int i = 0; i < m_MipmapCount; ++i)
	{
		DDSMipmap mipmap;
		mipmap.level = i;
		mipmap.width = width;
		mipmap.height = height;
		mipmap.texture_size = ((mipmap.width + 3) / 4) * ((mipmap.height + 3) / 4) * blockSize;
		mipmap.data = new uchar[mipmap.texture_size];
		memcpy(mipmap.data, buffer + header_size + offset, mipmap.texture_size);

		offset += mipmap.texture_size;
		width /= 2;
		height /= 2;

		mipmaps.push_back(mipmap);
	}

	delete[] buffer;
	m_Success = true;
}

void Rove::LoadDDS::Load(std::filesystem::path path)
{
	Load(path.string());
}