/** @file Mesh.cpp
 *  @brief 网格可绘制对象实现 - Mesh drawable object implementation
 *
 *  包含 Mesh 和 MaterialCBuffer 类的成员函数实现。
 *  Contains the member function implementations of Mesh and MaterialCBuffer classes.
 */
#include "Mesh.h"
#include "../Bindable/Rasterizer.h"

namespace YingLong
{
	/** @brief 构造函数
	 *  Constructor
	 *
	 *  从网格数据创建网格可绘制对象。
	 *  Creates a mesh drawable object from mesh data.
	 *
	 *  @param graphics 图形设备对象引用 / Graphics device object reference
	 *  @param data 网格数据 / Mesh data
	 */
	Mesh::Mesh(Graphics& graphics, const MeshData& data)
	{
		// 保存网格数据
		// Save mesh data
		this->Data = data;

		// 首次创建时初始化静态绑定（同类所有实例共享）
		// Initialize static bindings on first creation (shared by all instances of same type)
		if (!IsStaticInitialized())
		{
			
			// 创建顶点着色器并保存字节码用于输入布局
			// Create vertex shader and save bytecode for input layout
			ID3D10Blob* pVertexShaderByteCode = nullptr;
			auto pVertexShader = std::make_unique<VertexShader>(graphics,
				"CodeFile/Shader/PBRVertexShader.hlsl");
			pVertexShaderByteCode = pVertexShader->GetBytecode();
			AddStaticBind(std::move(pVertexShader));

			// 创建像素着色器
			// Create pixel shader
			AddStaticBind(std::make_unique<PixelShader>(graphics, 
				"CodeFile/Shader/PBRPixelShader.hlsl"));

			// 定义输入布局描述
			// Define input layout description
			const std::vector<D3D11_INPUT_ELEMENT_DESC> ied =
			{
				{ "Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "Normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TextureCoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};
			// 添加静态输入布局
			// Add static input layout
			AddStaticBind(std::make_unique<InputLayout>(graphics, ied, pVertexShaderByteCode));

			// 添加静态采样器
			// Add static sampler
			AddStaticBind(std::make_unique<Sampler>(graphics));
			// 设置图元拓扑为三角形列表
			// Set primitive topology to triangle list
			AddStaticBind(std::make_unique<Topology>(graphics, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST));

			// 实体光栅化状态：确保模型网格以实体模式渲染
			// Solid rasterizer state: ensure model mesh renders as solid
			AddStaticBind(std::make_unique<Rasterizer>(graphics,
				D3D11_FILL_SOLID, D3D11_CULL_BACK));
		}
		else
		{
			// 静态绑定已存在，从中查找索引缓冲区
			// Static bindings already exist, find index buffer from them
			FindIndexBufferFromStatic();
		}

		// 添加实例级顶点缓冲区
		// Add instance-level vertex buffer
		AddBind(std::make_unique<VertexBuffer>(graphics, data.Vertices));
		// 添加实例级索引缓冲区
		// Add instance-level index buffer
		AddIndexBuffer(std::make_unique<IndexBuffer>(graphics, data.Indices));

		// 添加漫反射纹理（槽0），如果存在
		// Add diffuse texture (slot 0), if exists
		if (data.DiffuseTexture)
		{
			AddBind(std::make_unique<Texture>(graphics, *data.DiffuseTexture.get()));
		}
		// 添加金属度纹理（槽1），如果存在
		// Add metallic texture (slot 1), if exists
		if (data.MetallicTexture)
		{
			AddBind(std::make_unique<Texture>(graphics, *data.MetallicTexture.get(), 1));
		}
		// 添加粗糙度纹理（槽2），如果存在
		// Add roughness texture (slot 2), if exists
		if (data.RoughnessTexture)
		{
			AddBind(std::make_unique<Texture>(graphics, *data.RoughnessTexture.get(), 2));
		}
		// 添加法线纹理（槽3），如果存在
		// Add normal texture (slot 3), if exists
		if (data.NormalTexture)
		{
			AddBind(std::make_unique<Texture>(graphics, *data.NormalTexture.get(), 3));
		}

		// 添加变换常量缓冲区
		// Add transform constant buffer
		AddBind(std::make_unique<TransformConstantBuffer>(graphics, *this));

		// 添加材质常量缓冲区
		// Add material constant buffer
		AddBind(std::make_unique<MaterialCBuffer>(graphics, this));
	}

	/** @brief 获取变换矩阵
	 *  Get transformation matrix
	 *
	 *  返回网格的世界变换矩阵，包含缩放、旋转和平移。
	 *  Returns the world transformation matrix of the mesh, including
	 *  scale, rotation, and translation.
	 *
	 *  @return DirectX 变换矩阵 / DirectX transformation matrix
	 */
	XMMATRIX Mesh::GetTransformXM() const noexcept
	{
		// 变换顺序：缩放 -> 旋转 -> 平移（标准 SRT）
		// Transformation order: scale -> rotation -> translation (standard SRT)
		return XMMatrixScaling(this->Scale.x, this->Scale.y, this->Scale.z) *
			XMMatrixRotationRollPitchYaw(
				(this->Rotation.x / 360.0f) * XM_2PI,
				(this->Rotation.y / 360.0f) * XM_2PI,
				(this->Rotation.z / 360.0f) * XM_2PI) *
			XMMatrixTranslation(this->Position.x, this->Position.y, this->Position.z);
	}

	/** @brief 绑定材质常量缓冲区
	 *  Bind material constant buffer
	 *
	 *  更新材质数据并绑定到像素着色器。
	 *  Updates material data and binds to pixel shader.
	 *
	 *  @param graphics 图形设备对象引用 / Graphics device object reference
	 */
	void MaterialCBuffer::Bind(Graphics& graphics) noexcept
	{
		// 设置默认材质参数（白色反照率，使纹理颜色正确显示）
		// Set default material parameters (white albedo so texture colors show correctly)
		this->CBufferData.Albedo = { 1.0f, 1.0f, 1.0f };
		this->CBufferData.Metallic = 0.1f;
		this->CBufferData.Roughness = 0.8f;
		this->CBufferData.AmbientOcclusion = 1.0f;

		// 根据父网格是否有对应纹理设置纹理使用标志
		// Set texture usage flags based on whether parent mesh has corresponding textures
		this->CBufferData.UseAlbedoTexture = this->Parent->Data.DiffuseTexture != nullptr;
		this->CBufferData.UseMetallicTexture = this->Parent->Data.MetallicTexture != nullptr;
		this->CBufferData.UseRoughnessTexture = this->Parent->Data.RoughnessTexture != nullptr;
		this->CBufferData.UseNormalTexture = this->Parent->Data.NormalTexture != nullptr;
		this->CBufferData.UseAOTexture = false;

		// 更新常量缓冲区数据
		// Update constant buffer data
		this->Update(graphics, this->CBufferData);

		// 调用基类绑定函数
		// Call base class bind function
		PixelConstantBuffer::Bind(graphics);
	}
}
