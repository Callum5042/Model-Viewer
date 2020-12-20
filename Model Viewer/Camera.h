#pragma once

#include <DirectXMath.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class ICamera
{
public:
	ICamera() = default;
	virtual ~ICamera() = default;
};

class Camera : public ICamera
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

class GlCamera : public ICamera
{
public:
	GlCamera(int width, int height);
	virtual ~GlCamera() = default;

	constexpr glm::mat4 GetView() { return m_View; }
	constexpr glm::mat4 GetProjection() { return m_Projection; }

	void Update();
	void Resize(int width, int height);

private:
	glm::vec3 m_Position = glm::vec3(0, 0, -5);
	glm::mat4 m_View = glm::mat4(1.0f);
	glm::mat4 m_Projection = glm::mat4(1.0f);

	float m_FOV = 50.0f;

	float m_Pitch = 30.0f;
	float m_Yaw = 90.0f;
};