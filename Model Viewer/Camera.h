#pragma once

#include <DirectXMath.h>

class Camera
{
public:
	Camera(int width, int height);

	void Resize(int width, int height);

	constexpr DirectX::XMMATRIX GetView() { return m_View; }
	constexpr DirectX::XMMATRIX GetProjection() { return m_Projection; }

	void Update(float yaw, float pitch);
	void UpdateFov(float fov);

private:
	DirectX::XMMATRIX m_View;
	DirectX::XMMATRIX m_Projection;
	DirectX::XMFLOAT3 m_Position;

	float m_FOV = 50.0;

	float m_Pitch = 30.0f;
	float m_Yaw = 0.0f;

	int m_WindowWidth = 0;
	int m_WindowHeight = 0;
};