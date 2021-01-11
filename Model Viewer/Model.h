#pragma once

#include "Pch.h"
#include <map>
#include <DirectXMath.h>
class IRenderer;
class DxRenderer;
class Camera;
class ICamera;
class GlShader;
class IShader;
class GlCamera;

struct Position
{
	float x = 0;
	float y = 0;
	float z = 0;
};

struct Colour
{
	float r = 0.0f;
	float g = 0.0f;
	float b = 0.0f;
	float a = 0.0f;
};

struct Texture
{
	float u = 0;
	float v = 0;
};

struct Normal
{
	float x = 0;
	float y = 0;
	float z = 0;
};

struct Tangent
{
	float x = 0;
	float y = 0;
	float z = 0;
};

struct BiTangent
{
	float x = 0;
	float y = 0;
	float z = 0;
};

struct Vertex
{
	Vertex() {}

	Position position = {};
	Colour colour = {};
	Texture texture = {};
	Normal normal = {};
	Tangent tangent = {};
	BiTangent bi_tangent = {};

	// Weights
	float weight[4] = { 0, 0, 0, 0 };

	// Bone index
	int bone[4] = { 0, 0, 0, 0 };
};

struct BoneInfo
{
	int parentId = 0;
	std::string name;
	std::string parentName;
	DirectX::XMMATRIX offset;
};

struct Subset
{
	unsigned totalIndex = 0;
	unsigned startIndex = 0;
	unsigned baseVertex = 0;
};

///<summary>
	/// A Keyframe defines the bone transformation at an instant in time.
	///</summary>
struct Keyframe
{
	Keyframe() = default;
	virtual ~Keyframe() = default;

	float TimePos = 0.0f;
	DirectX::XMFLOAT3 Translation = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	DirectX::XMFLOAT3 Scale = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
	DirectX::XMFLOAT4 RotationQuat = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
};

///<summary>
/// A BoneAnimation is defined by a list of keyframes.  For time
/// values inbetween two keyframes, we interpolate between the
/// two nearest keyframes that bound the time.  
///
/// We assume an animation always has two keyframes.
///</summary>
struct BoneAnimation
{
	float GetStartTime() const;
	float GetEndTime() const;

	void Interpolate(float t, DirectX::XMMATRIX& M) const;

	std::vector<Keyframe> Keyframes;
};

///<summary>
/// Examples of AnimationClips are "Walk", "Run", "Attack", "Defend".
/// An AnimationClip requires a BoneAnimation for every bone to form
/// the animation clip.    
///</summary>
struct AnimationClip
{
	float GetClipStartTime() const;
	float GetClipEndTime() const;

	void Interpolate(float t, std::vector<DirectX::XMMATRIX>& boneTransforms)const;

	std::vector<BoneAnimation> BoneAnimations;
};

struct MeshData
{
	MeshData() = default;
	virtual ~MeshData() = default;

	std::vector<Vertex> vertices;
	std::vector<UINT> indices;
	std::vector<Subset> subsets;
	std::vector<BoneInfo> bones;
	std::map<std::string, AnimationClip> animations;
};

class IModel
{
public:
	IModel() = default;
	virtual ~IModel() = default;

	virtual bool Load(const std::string& path) = 0;
	virtual void Update(float dt) = 0;
	virtual void Render(ICamera* camera) = 0;
};

class DxModel : public IModel
{
public:
	DxModel(IRenderer* renderer);
	virtual ~DxModel();

	bool Load(const std::string& path) override;
	void Update(float dt) override;
	void Render(ICamera* camera) override;

private:
	DxRenderer* m_Renderer = nullptr;
	std::unique_ptr<MeshData> m_MeshData = nullptr;

	// Model buffers
	ComPtr<ID3D11Buffer> m_VertexBuffer = nullptr;
	ComPtr<ID3D11Buffer> m_IndexBuffer = nullptr;
	ComPtr<ID3D11Buffer> m_ConstantBuffer = nullptr;

	// Texture resources
	ComPtr<ID3D11ShaderResourceView> m_DiffuseTexture = nullptr;
	ComPtr<ID3D11ShaderResourceView> m_NormalTexture = nullptr;

	// Light
	ComPtr<ID3D11Buffer> m_LightBuffer = nullptr;

	// Bones
	ComPtr<ID3D11Buffer> m_BoneConstantBuffer = nullptr;
};

class GlModel : public IModel
{
public:
	GlModel(IShader* shader);
	virtual ~GlModel();

	bool Load(const std::string& path) override;
	void Update(float dt) override;
	void Render(ICamera* camera) override;

private:
	GlShader* m_Shader = nullptr;
	std::unique_ptr<MeshData> m_MeshData = nullptr;

	// Mode buffers
	GLuint m_VertexArrayObject = 0;
	GLuint m_VertexBuffer = 0;
	GLuint m_IndexBuffer = 0;

	// Texture resourcees
	GLuint m_DiffuseTextureId = 0;
	GLuint m_NormalTextureId = 0;
};