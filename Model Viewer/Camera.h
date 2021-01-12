#pragma once

#include <DirectXMath.h>

class Camera final
{
public:
	Camera(int width, int height, float fov) noexcept;

	// Resize camera projection
	void Resize(int width, int height);

	// Get the view matrix
	constexpr DirectX::XMMATRIX GetView() { return m_View; }

	// Get the projection matrix
	constexpr DirectX::XMMATRIX GetProjection() { return m_Projection; }

	// Get the current camera position in world space
	constexpr DirectX::XMFLOAT3 GetPosition() { return m_Position; }

	// Set pitch and yaw
	void SetPitchAndYaw(float pitch, float yaw);

	// Update field of view
	void SetFov(float fov);

	// Distance from centre point (0, 0, 0)
	void SetRadius(float radius);

private:
	DirectX::XMFLOAT3 m_Position;
	DirectX::XMMATRIX m_View;
	DirectX::XMMATRIX m_Projection;

	// Window size
	int m_WindowWidth = 0;
	int m_WindowHeight = 0;

	// Distance from point
	float m_Radius = -8.0f;

	// Camera settings
	float m_FOV = 0.0;
	float m_Pitch = 0.0f;
	float m_Yaw = 0.0f;
};