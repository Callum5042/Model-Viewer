#include "Pch.h"
#include "Camera.h"
#include <algorithm>

Camera::Camera(int width, int height, float fov) noexcept : m_WindowWidth(width), m_WindowHeight(height), m_FOV(fov)
{
	SetFov(fov);
	Resize(width, height);
}

void Camera::Resize(int width, int height)
{
	auto fieldOfView = DirectX::XMConvertToRadians(m_FOV);
	auto screenAspect = static_cast<float>(width) / height;
	m_Projection = DirectX::XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, 0.01f, 100.0f);

	// Keep a copy of width and height for updating the FOV
	m_WindowWidth = width;
	m_WindowHeight = height;
}

void Camera::SetPitchAndYaw(float pitch, float yaw)
{
	// Cache pitch and yaw for when the radius is changed
	m_Pitch = pitch;
	m_Yaw = yaw;

	// Convert degrees to radians
	auto pitch_radians = DirectX::XMConvertToRadians(pitch);
	auto yaw_radians = DirectX::XMConvertToRadians(yaw);

	// Calculate rotation matrix from pitch and yaw
	auto position = DirectX::XMVectorSet(0.0f, 0.0f, m_Radius, 0.0f);
	auto camRotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(pitch_radians, yaw_radians, 0);
	position = XMVector3TransformCoord(position, camRotationMatrix);
	XMStoreFloat3(&m_Position, position);

	// Look at
	auto eye = position;
	auto at = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	auto up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	m_View = DirectX::XMMatrixLookAtLH(eye, at, up);
}

void Camera::SetFov(float fov)
{
	m_FOV = fov;
	Resize(m_WindowWidth, m_WindowHeight);
}

void Camera::SetRadius(float radius)
{
	m_Radius = radius;
	SetPitchAndYaw(m_Pitch, m_Yaw);
}