/** @file SolidSphere.cpp
 *  @brief 纯色球体可绘制对象实现 - Solid-color sphere drawable object implementation
 *
 *  包含 SolidSphereDrawable 类的成员函数实现。
 *  Contains the member function implementations of the SolidSphereDrawable class.
 */
#include "SolidSphere.h"
#include "../Bindable/ConstantBuffers.h"
#include "../Bindable/Rasterizer.h"

namespace YingLong
{
	/** @brief 拷贝构造函数
	 *  Copy constructor
	 *
	 *  @param 要拷贝的球体对象 / The sphere object to copy from
	 */
	SolidSphereDrawable::SolidSphereDrawable(const SolidSphereDrawable&)
	{

	}

	/** @brief 构造函数
	 *  Constructor
	 *
	 *  使用指定的半径和颜色创建球体。
	 *  Creates a sphere with specified radius and color.
	 *
	 *  @param graphics 图形设备对象引用 / Graphics device object reference
	 *  @param radius 球体半径 / Sphere radius
	 *  @param color 球体颜色 / Sphere color
	 */
	SolidSphereDrawable::SolidSphereDrawable(Graphics& graphics, float radius, XMFLOAT3 color)
	{
		// 首次创建时初始化静态绑定（同类所有实例共享）
		// Initialize static bindings on first creation (shared by all instances of same type)
		if (!IsStaticInitialized())
		{
			// 定义顶点结构
			// Define vertex structure
			struct Vertex
			{
				XMFLOAT3 Position; ///< 位置 / Position
				XMFLOAT3 Color;    ///< 颜色 / Color
			};
			// 生成球体模型
			// Generate sphere model
			auto Model = Sphere::Make<Vertex>();
			// 缩放球体到指定半径
			// Scale sphere to specified radius
			Model.Transform(XMMatrixScaling(radius, radius, radius));
			// 设置顶点颜色
			// Set vertex colors
			for (auto& v : Model.vertices)
			{
				v.Color = color;
			}
			// 添加静态顶点缓冲区
			// Add static vertex buffer
			AddStaticBind(std::make_unique<VertexBuffer>(graphics, Model.vertices));
			// 添加静态索引缓冲区
			// Add static index buffer
			AddStaticIndexBuffer(std::make_unique<IndexBuffer>(graphics, Model.indices));

			// 创建顶点着色器并保存字节码用于输入布局
			// Create vertex shader and save bytecode for input layout
			auto pVertexShader = std::make_unique<VertexShader>(graphics,
				"CodeFile/Shader/SolidVertexShader.hlsl");
			auto pVertexShaderByteCode = pVertexShader->GetBytecode();
			AddStaticBind(std::move(pVertexShader));

			// 创建像素着色器
			// Create pixel shader
			AddStaticBind(std::make_unique<PixelShader>(graphics,
				"CodeFile/Shader/SolidPixelShader.hlsl"));

			// 定义输入布局描述
			// Define input layout description
			const std::vector<D3D11_INPUT_ELEMENT_DESC> ied =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
			};
			// 添加静态输入布局
			// Add static input layout
			AddStaticBind(std::make_unique<InputLayout>(graphics, ied, pVertexShaderByteCode));

			// 设置图元拓扑为三角形列表
			// Set primitive topology to triangle list
			AddStaticBind(std::make_unique<Topology>(graphics, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST));

			// 实体光栅化状态：确保纯色球体以实体模式渲染（点光源可视化）
			// Solid rasterizer state: ensure solid sphere renders as solid (point light visualization)
			AddStaticBind(std::make_unique<Rasterizer>(graphics,
				D3D11_FILL_SOLID, D3D11_CULL_BACK));
		}
		else
		{
			// 静态绑定已存在，从中查找索引缓冲区
			// Static bindings already exist, find index buffer from them
			FindIndexBufferFromStatic();
		}

		// 添加变换常量缓冲区
		// Add transform constant buffer
		AddBind(std::make_unique<TransformConstantBuffer>(graphics, *this));
		// 添加纯色常量缓冲区
		// Add solid color constant buffer
		AddBind(std::make_unique<SolidColorConstantBuffer>(graphics, &GetColor()));
	}

	/** @brief 设置位置
	 *  Set position
	 *
	 *  @param Position 新位置 / New position
	 */
	void SolidSphereDrawable::SetPosition(DirectX::XMFLOAT3 Position) noexcept
	{
		this->Position = Position;
	}

	/** @brief 设置颜色
	 *  Set color
	 *
	 *  设置实体球体的颜色。
	 *  如果需要动态更新，可能需要刷新常量缓冲区。
	 *
	 *  Sets the color of the solid sphere.
	 *  If dynamic update is needed, the constant buffer may need to be refreshed.
	 *
	 *  @param color 新颜色 / New color
	 */
	void SolidSphereDrawable::SetColor(DirectX::XMFLOAT3 color) noexcept
	{
		this->Color = color;
	}

	/** @brief 更新球体状态
	 *  Update sphere state
	 *
	 *  @param dt 时间增量（秒） / Time delta in seconds
	 *  @param aspect 宽高比 / Aspect ratio
	 */
	void SolidSphereDrawable::Update(float dt, float aspect) noexcept
	{

	}

	/** @brief 获取变换矩阵
	 *  Get transformation matrix
	 *
	 *  返回球体的世界变换矩阵。
	 *  Returns the world transformation matrix of the sphere.
	 *
	 *  @return DirectX 变换矩阵 / DirectX transformation matrix
	 */
	DirectX::XMMATRIX SolidSphereDrawable::GetTransformXM() const noexcept
	{
		// 仅平移，无旋转和缩放（半径已烘焙到几何体中）
		// Translation only, no rotation or scale (radius baked into geometry)
		return DirectX::XMMatrixTranslation(this->Position.x, this->Position.y, this->Position.z);
	}

	/** @brief 获取颜色
	 *  Get color
	 *
	 *  @return 颜色的常量引用 / Const reference to color
	 */
	const XMFLOAT3& SolidSphereDrawable::GetColor() const noexcept
	{
		return this->Color;
	}
}
