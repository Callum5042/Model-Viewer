#pragma once

#include <DirectXMath.h>

class Camera final
{
public:
	Camera(int width, int height, float fov);

	void Resize(int width, int height);

	constexpr DirectX::XMMATRIX GetView() { return m_View; }
	constexpr DirectX::XMMATRIX GetProjection() { return m_Projection; }
	constexpr DirectX::XMFLOAT3 GetPosition() { return m_Position; }

	void SetPitchAndYaw(float pitch, float yaw);
	void SetFov(float fov);

	void SetRadius(float radius)
	{
		m_Radius = radius;
	}

private:
	DirectX::XMFLOAT3 m_Position;
	DirectX::XMMATRIX m_View;
	DirectX::XMMATRIX m_Projection;

	float m_FOV = 0.0;

	int m_WindowWidth = 0;
	int m_WindowHeight = 0;

	float m_Radius = -8.0f;
};