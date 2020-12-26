#include "Pch.h"
#include "Camera.h"
#include <algorithm>
using namespace DirectX;
using namespace glm;

Camera::Camera(int width, int height, float fov) : m_WindowWidth(width), m_WindowHeight(height), m_FOV(fov)
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
	// Convert degrees to radians
	auto pitch_radians = DirectX::XMConvertToRadians(pitch);
	auto yaw_radians = DirectX::XMConvertToRadians(yaw);

	// Calculate rotation matrix from pitch and yaw
	auto position = DirectX::XMVectorSet(0.0f, 0.0f, -8.0f, 0.0f);
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

GlCamera::GlCamera(int width, int height, float fov) : m_WindowWidth(width), m_WindowHeight(height), m_FOV(fov)
{
	SetFov(fov);
	Resize(width, height);
}

void GlCamera::Resize(int width, int height)
{
	m_WindowWidth = width;
	m_WindowHeight = height;

	auto fieldOfView = glm::radians(m_FOV);
	auto screenAspect = static_cast<float>(width) / height;
	m_Projection = glm::perspective(fieldOfView, screenAspect, 0.01f, 100.0f);
}

void GlCamera::SetPitchAndYaw(float pitch, float yaw)
{
	// Convert degrees to radians
	auto pitch_radians = glm::radians(pitch);
	auto yaw_radians = glm::radians(yaw);

	// Calculate rotation matrix from pitch and yaw
	glm::vec4 position(0.0f, 0.0f, -8.0f, 0.0f);
	auto rotationMatrix = glm::yawPitchRoll(yaw_radians, pitch_radians, 0.0f);
	position = rotationMatrix * position;

	// Look at
	glm::vec3 eye = position;
	glm::vec3 at(0.0f, 0.0f, 0.0f);
	glm::vec3 up(0.0f, 1.0f, 0.0f);
	m_View = glm::lookAt(eye, at, up);

	const vec3 f(normalize(at - eye));
	const vec3 s(normalize(cross(up, f)));
	const vec3 u(cross(f, s));

	mat4 Result(1);
	Result[0][0] = s.x;
	Result[1][0] = s.y;
	Result[2][0] = s.z;
	Result[0][1] = u.x;
	Result[1][1] = u.y;
	Result[2][1] = u.z;
	Result[0][2] = -f.x;
	Result[1][2] = -f.y;
	Result[2][2] = -f.z;
	Result[3][0] = -dot(s, eye);
	Result[3][1] = -dot(u, eye);
	Result[3][2] = dot(f, eye);

	m_View = Result;
}

void GlCamera::SetFov(float fov)
{
	m_FOV = fov;
	Resize(m_WindowWidth, m_WindowHeight);
}
