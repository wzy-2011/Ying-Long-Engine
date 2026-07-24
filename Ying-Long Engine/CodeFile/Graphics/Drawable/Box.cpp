/** @file Box.cpp
 *  @brief 立方体可绘制对象实现 - Box drawable object implementation
 *
 *  包含 BoxDrawable 类的成员函数实现。
 *  Contains the member function implementations of the BoxDrawable class.
 */
#include "Box.h"
#include "../Bindable/VertexBuffer.h"
#include "../Bindable/Rasterizer.h"
#include "../Bindable/Sampler.h"

namespace YingLong
{
	/** @brief 构造函数
	 *  Constructor
	 *
	 *  使用随机数生成器创建立方体，初始化静态绑定（首次创建时）
	 *  和实例级绑定。
	 *
	 *  Creates a box using random number generators, initializing static
	 *  bindings (on first creation) and instance-level bindings.
	 *
	 *  @param graphics 图形设备对象引用 / Graphics device object reference
	 *  @param rng 随机数生成器引用 / Random number generator reference
	 *  @param adist 角度分布（用于初始角度） / Angle distribution (for initial angles)
	 *  @param ddist 角速度分布（用于旋转速度） / Angular velocity distribution (for rotation speed)
	 *  @param odist 轨道分布 / Orbit distribution
	 *  @param rdist 半径分布（用于轨道半径） / Radius distribution (for orbit radius)
	 */
	BoxDrawable::BoxDrawable(Graphics& graphics,
		std::mt19937& rng,
		std::uniform_real_distribution<float>& adist,
		std::uniform_real_distribution<float>& ddist,
		std::uniform_real_distribution<float>& odist,
		std::uniform_real_distribution<float>& rdist)
		: r(rdist(rng)),
		droll(ddist(rng)),
		dpitch(ddist(rng)),
		dyaw(ddist(rng)),
		dphi(ddist(rng)),
		dchi(ddist(rng)),
		chi(adist(rng)),
		theta(adist(rng)),
		phi(adist(rng))
	{
		// 首次创建时初始化静态绑定（同类所有实例共享）
		// Initialize static bindings on first creation (shared by all instances of same type)
		if (!IsStaticInitialized())
		{
			// 定义顶点结构
			// Define vertex structure
			struct Vertex
			{
				XMFLOAT3 Position;       ///< 位置 / Position
				XMFLOAT3 Normal;         ///< 法线 / Normal
				XMFLOAT2 TextureCoord;   ///< 纹理坐标 / Texture coordinate
			};
			// 生成带蒙皮的立方体模型
			// Generate skinned cube model
			auto Model = Cube::MakeSkinned<Vertex>();
			// 设置独立平面法线（每个面法线独立）
			// Set independent flat normals (each face has independent normals)
			Model.SetNormalsIndependentFlat();
			// 添加静态顶点缓冲区
			// Add static vertex buffer
			AddStaticBind(std::make_unique<VertexBuffer>(graphics, Model.vertices));


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


			// 添加静态索引缓冲区
			// Add static index buffer
			AddStaticIndexBuffer(std::make_unique<IndexBuffer>(graphics, Model.indices));


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


			// 设置图元拓扑为三角形列表
			// Set primitive topology to triangle list
			AddStaticBind(std::make_unique<Topology>(graphics, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST));

			// 实体光栅化状态：确保立方体以实体模式渲染，不受聚光灯线框模式影响
			// Solid rasterizer state: ensure box renders as solid, not affected by spotlight wireframe mode
			AddStaticBind(std::make_unique<Rasterizer>(graphics,
				D3D11_FILL_SOLID, D3D11_CULL_BACK));

			// 添加静态采样器：确保纹理正确采样
			// Add static sampler: ensure correct texture sampling
			AddStaticBind(std::make_unique<Sampler>(graphics));
		}
		else
		{
			// 静态绑定已存在，从中查找索引缓冲区
			// Static bindings already exist, find index buffer from them
			FindIndexBufferFromStatic();
		}

		// 添加实例级纹理
		// Add instance-level texture
		std::unique_ptr<Texture> texture =
			std::make_unique<Texture>(graphics, Surface("Resources/Icon/Ying-Long.jpg"));
		AddBind(std::move(texture));

		// 添加变换常量缓冲区
		// Add transform constant buffer
		AddBind(std::make_unique<TransformConstantBuffer>(graphics, *this));
	};

	/** @brief 更新立方体状态
	 *  Update box state
	 *
	 *  根据时间增量更新旋转角度和保存宽高比。
	 *  Updates rotation angles based on time delta and saves aspect ratio.
	 *
	 *  @param dt 时间增量（秒） / Time delta in seconds
	 *  @param aspect 宽高比 / Aspect ratio
	 */
	void BoxDrawable::Update(float dt, float aspect) noexcept
	{
		// 更新各旋转角度
		// Update each rotation angle
		roll += droll * dt;
		pitch += dpitch * dt;
		yaw += dyaw * dt;
		theta += dtheta * dt;
		phi += dphi * dt;
		chi += dchi * dt;

		// 保存宽高比
		// Save aspect ratio
		this->Aspect = aspect;
	}

	/** @brief 获取变换矩阵
	 *  Get transformation matrix
	 *
	 *  返回立方体的世界变换矩阵，包含轨道平移、旋转和位置平移。
	 *  Returns the world transformation matrix of the box, including
	 *  orbital translation, rotation, and position translation.
	 *
	 *  @return DirectX 变换矩阵 / DirectX transformation matrix
	 */
	DirectX::XMMATRIX BoxDrawable::GetTransformXM() const noexcept
	{
		// 变换顺序：轨道半径偏移 -> 旋转 -> Z轴平移
		// Transformation order: orbit radius offset -> rotation -> Z axis translation
		return DirectX::XMMatrixTranslation(r, 0.0f, 0.0f) *
			DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll) *
			DirectX::XMMatrixTranslation(0.0f, 0.0f, 20.0f);
	}
}
