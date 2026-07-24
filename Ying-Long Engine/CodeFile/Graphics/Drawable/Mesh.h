/** @file Mesh.h
 *  @brief 网格可绘制对象 - Mesh drawable object
 *
 *  包含网格可绘制对象和 PBR 材质常量缓冲区类定义。
 *  Contains mesh drawable object and PBR material constant buffer class definitions.
 */
#pragma once
#include "../ModelImporter/ModelImporter.h"
#include "DrawableBase.h"
#include "../Bindable/InputLayout.h"
#include "../Bindable/TransformConstantBuffer.h"
#include "../Bindable/VertexShader.h"
#include "../Bindable/PixelShader.h"
#include "../Bindable/Topology.h"
#include "../Bindable/Sampler.h"
#include "../Bindable/Texture.h"
#include "../Light/PointLight.h"

namespace YingLong
{
	/** @brief PBR 材质常量缓冲区数据结构
	 *  PBR material constant buffer data structure
	 *
	 *  存储 PBR 材质所需的所有参数，包括反照率、粗糙度、金属度、
	 *  环境光遮蔽以及各纹理的使用标志。
	 *
	 *  Stores all parameters required for PBR material, including albedo,
	 *  roughness, metallic, ambient occlusion, and usage flags for each texture.
	 */
	struct MaterialCBufferData
	{
		MaterialCBufferData()
		{
			this->Albedo = { 0.0f, 0.0f, 0.0f };
			this->Metallic = 0.8f;
			this->Roughness = 0.5f;
			this->AmbientOcclusion = 1.0f;
			this->UseAlbedoTexture = 0;
			this->UseRoughnessTexture = 0;
			this->UseMetallicTexture = 0;
			this->UseNormalTexture = 0;
			this->UseAOTexture = 0;
		}

		MaterialCBufferData(const MaterialCBufferData& other) = default;

		XMFLOAT3 Albedo;          ///< 反照率颜色 / Albedo color
		float Metallic;           ///< 金属度 / Metallic
		float Roughness;          ///< 粗糙度 / Roughness
		float AmbientOcclusion;   ///< 环境光遮蔽 / Ambient occlusion
		int UseAlbedoTexture;     ///< 是否使用反照率纹理 / Whether to use albedo texture
		int UseRoughnessTexture;  ///< 是否使用粗糙度纹理 / Whether to use roughness texture
		int UseMetallicTexture;   ///< 是否使用金属度纹理 / Whether to use metallic texture
		int UseNormalTexture;     ///< 是否使用法线纹理 / Whether to use normal texture
		int UseAOTexture;         ///< 是否使用环境光遮蔽纹理 / Whether to use AO texture
	};

	class Mesh;

	/** @brief PBR 材质常量缓冲区类
	 *  PBR material constant buffer class
	 *
	 *  用于将 PBR 材质数据绑定到像素着色器的常量缓冲区。
	 *  Constant buffer for binding PBR material data to the pixel shader.
	 */
	class MaterialCBuffer : public PixelConstantBuffer<MaterialCBufferData>
	{
	public:
		/** @brief 默认构造函数
		 *  Default constructor
		 */
		MaterialCBuffer() : PixelConstantBuffer<MaterialCBufferData>()
		{

		}

		/** @brief 构造函数
		 *  Constructor
		 *
		 *  @param graphics 图形设备对象引用 / Graphics device object reference
		 *  @param parent 父网格对象指针 / Parent mesh object pointer
		 */
		MaterialCBuffer(Graphics& graphics, const Mesh* parent) 
			: PixelConstantBuffer<MaterialCBufferData>(graphics, 1)
		{
			this->Parent = parent;
		}

		/** @brief 绑定材质常量缓冲区
		 *  Bind material constant buffer
		 *
		 *  更新材质数据并绑定到像素着色器。
		 *  Updates material data and binds to pixel shader.
		 *
		 *  @param graphics 图形设备对象引用 / Graphics device object reference
		 */
		void Bind(Graphics& graphics) noexcept override;

	private:
		const Mesh* Parent = nullptr; ///< 父网格对象指针 / Parent mesh object pointer
	};

	/** @brief 网格可绘制对象类
	 *  Mesh drawable object class
	 *
	 *  从模型数据创建的可绘制网格对象，支持 PBR 材质和多纹理。
	 *  每个网格实例有自己的顶点缓冲区、索引缓冲区和纹理，
	 *  但共享着色器、输入布局等静态资源。
	 *
	 *  Drawable mesh object created from model data, supporting PBR material
	 *  and multiple textures. Each mesh instance has its own vertex buffer,
	 *  index buffer, and textures, but shares static resources such as
	 *  shaders and input layouts.
	 */
	class Mesh : public DrawableBase<Mesh>
	{
	public:
		/** @brief 默认构造函数
		 *  Default constructor
		 */
		Mesh() = default;

		/** @brief 构造函数
		 *  Constructor
		 *
		 *  从网格数据创建网格可绘制对象。
		 *  Creates a mesh drawable object from mesh data.
		 *
		 *  @param graphics 图形设备对象引用 / Graphics device object reference
		 *  @param data 网格数据 / Mesh data
		 */
		Mesh(Graphics& graphics, const MeshData& data);

		/** @brief 拷贝构造函数
		 *  Copy constructor
		 */
		Mesh(const Mesh&) = default;

		/** @brief 更新网格状态
		 *  Update mesh state
		 *
		 *  @param dt 时间增量（秒） / Time delta in seconds
		 *  @param aspect 宽高比 / Aspect ratio
		 */
		void Update(float dt, float aspect) noexcept override  {}

		/** @brief 获取变换矩阵
		 *  Get transformation matrix
		 *
		 *  返回网格的世界变换矩阵，包含缩放、旋转和平移。
		 *  Returns the world transformation matrix of the mesh, including
		 *  scale, rotation, and translation.
		 *
		 *  @return DirectX 变换矩阵 / DirectX transformation matrix
		 */
		XMMATRIX GetTransformXM() const noexcept override;

	private:
		XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f }; ///< 位置 / Position
		XMFLOAT3 Rotation = { 0.0f, 0.0f, 0.0f }; ///< 旋转（度） / Rotation (degrees)
		XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f };    ///< 缩放 / Scale
		MeshData Data;                             ///< 网格数据 / Mesh data

		friend class Model;            ///< 模型类友元 / Model class friend
		friend class MaterialCBuffer;  ///< 材质常量缓冲区友元 / Material constant buffer friend
	};
}
