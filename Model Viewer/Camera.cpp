#include "Pch.h"
#include "Camera.h"
#include <algorithm>

Camera::Camera(int width, int height)
{
	m_Position = DirectX::XMFLOAT3(0.0f, 0.0f, -8.0f);

	Update(0, 0);
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

void Camera::Update(float yaw, float pitch)
{
	m_Yaw += (yaw * 0.25f);
	m_Pitch += (pitch * 0.25f);

	m_Pitch = std::clamp<float>(m_Pitch, -89, 89);

	DirectX::XMVECTOR position = DirectX::XMLoadFloat3(&m_Position);
	DirectX::XMMATRIX camRotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(DirectX::XMConvertToRadians(m_Pitch), DirectX::XMConvertToRadians(m_Yaw), 0);
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
	Update();
	Resize(width, height);
}

void GlCamera::Update()
{
	float distance = 8.0f;

	glm::vec3 direction;
	direction.x = distance * cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
	direction.y = distance * sin(glm::radians(m_Pitch));
	direction.z = distance * sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));

	m_Position = direction;

	glm::vec3 eye = m_Position = direction;
	glm::vec3 at(0, 0, 0);
	glm::vec3 up(0, 1, 0);

	m_View = glm::lookAt(eye, at, up);
}

void GlCamera::Resize(int width, int height)
{
	m_Projection = glm::perspective(glm::radians(m_FOV), ((float)width / height), 0.01f, 100.0f);
}