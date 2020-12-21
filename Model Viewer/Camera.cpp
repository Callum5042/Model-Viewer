#include "Pch.h"
#include "Camera.h"
#include <algorithm>

Camera::Camera(int width, int height)
{
	m_Position = DirectX::XMFLOAT3(0.0f, 0.0f, -8.0f);

	Resize(width, height);
}

void Camera::Resize(int width, int height)
{
	float fieldOfView = DirectX::XMConvertToRadians(m_FOV);
	float screenAspect = (float)width / (float)height;
	m_Projection = DirectX::XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, 0.01f, 100.0f);

	m_WindowWidth = width;
	m_WindowHeight = height;
}

void Camera::SetPitchAndYaw(float pitch, float yaw)
{
	DirectX::XMVECTOR position = DirectX::XMLoadFloat3(&m_Position);
	DirectX::XMMATRIX camRotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(DirectX::XMConvertToRadians(pitch), DirectX::XMConvertToRadians(yaw), 0);
	position = XMVector3TransformCoord(position, camRotationMatrix);

	DirectX::XMVECTOR eye = position;
	DirectX::XMVECTOR at = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	m_View = DirectX::XMMatrixLookAtLH(eye, at, up);
}

void Camera::UpdateFov(float fov)
{
	m_FOV -= fov;
	m_FOV = std::clamp<float>(m_FOV, 1.0f, 180.0f);

	Resize(m_WindowWidth, m_WindowHeight);
}

GlCamera::GlCamera(int width, int height)
{
	Resize(width, height);
}

void GlCamera::Resize(int width, int height)
{
	m_Projection = glm::perspective(glm::radians(m_FOV), ((float)width / height), 0.01f, 100.0f);
}

void GlCamera::SetPitchAndYaw(float pitch, float yaw)
{
	float distance = 8.0f;

	glm::vec3 direction;
	direction.x = distance * cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = distance * sin(glm::radians(pitch));
	direction.z = distance * sin(glm::radians(yaw)) * cos(glm::radians(pitch));

	m_Position = direction;

	glm::vec3 eye = m_Position = direction;
	glm::vec3 at(0, 0, 0);
	glm::vec3 up(0, 1, 0);

	m_View = glm::lookAt(eye, at, up);
}
