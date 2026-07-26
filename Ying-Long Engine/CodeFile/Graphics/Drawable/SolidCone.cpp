/** @file SolidCone.cpp
 *  @brief 纯色锥体可绘制对象实现（线框模式）/ Solid-color cone drawable object implementation (wireframe mode)
 *
 *  包含 SolidConeDrawable 类的成员函数实现。
 *  锥体以线框模式渲染，便于在编辑器中识别聚光灯方向。
 *  Contains the member function implementations of the SolidConeDrawable class.
 *  Cones are rendered in wireframe mode for easy spotlight direction identification.
 */
#include "SolidCone.h"
#include "../Bindable/ConstantBuffers.h"
#include "../Bindable/Rasterizer.h"

namespace YingLong
{
	/** @brief 拷贝构造函数
	 *  Copy constructor
	 *
	 *  由 DrawableBase 拷贝构造函数后，需要重新查找静态索引缓冲区。
	 *  注意：Drawable 基类的拷贝构造函数为空，不复制实例级绑定（TransformCB/ColorCB），
	 *        因此拷贝后的对象需要手动设置这些绑定才能正确渲染。
	 *  After DrawableBase copy, must re-find static index buffer.
	 *  Note: Drawable base copy ctor is empty, instance-level binds (TransformCB/ColorCB)
	 *        are NOT copied, so the copied object needs manual bind setup to render correctly.
	 *
	 *  @param 要拷贝的锥体对象 / The cone object to copy from
	 */
	SolidConeDrawable::SolidConeDrawable(const SolidConeDrawable&)
	{
		FindIndexBufferFromStatic();
		// 注意：实例绑定（TransformCB / ColorCB）不会被拷贝，
		// 需要调用者通过 SetPosition/SetColor 后重新设置。
		// Note: Instance binds (TransformCB / ColorCB) are not copied,
		// they must be re-set by caller via SetPosition/SetColor.
	}

	/** @brief 构造函数
	 *  Constructor
	 *
	 *  使用指定的高度、半径和颜色创建锥体。
	 *  Creates a cone with specified height, radius and color.
	 *
	 *  @param graphics 图形设备对象引用 / Graphics device object reference
	 *  @param height 锥体高度 / Cone height
	 *  @param radius 底面半径 / Base radius
	 *  @param color 锥体颜色 / Cone color
	 */
	SolidConeDrawable::SolidConeDrawable(Graphics& graphics, float height, float radius, XMFLOAT3 color)
		: ConeHeight(height), ConeRadius(radius)
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

			// 生成锥体模型
			// Generate cone model
			auto Model = Cone::Generate<Vertex>(32, ConeHeight, ConeRadius);
			// 设置顶点颜色
			// Set vertex colors
			for (auto& v : Model.vertices)
			{
				v.Color = color;
			}
			// 添加静态顶点缓冲区
			// Add static vertex buffer
			AddStaticBind(std::make_unique<VertexBuffer>(graphics, Model.vertices));

			// 生成线段索引（只渲染侧面边和底面圆周线，与 DX12 WireframeCone 一致）
			// Generate line indices (only side edges and base circumference, matching DX12 WireframeCone)
			// 顶点 0 是 apex，顶点 1..32 是底面边缘顶点
			// Vertex 0 is apex, vertices 1..32 are base rim vertices
			const UINT segments = 32;
			std::vector<unsigned short> lineIndices;
			for (UINT i = 0; i < segments; ++i)
			{
				unsigned short rim0 = 1 + static_cast<unsigned short>(i);
				unsigned short rim1 = 1 + static_cast<unsigned short>((i + 1) % segments);
				// 侧面边线：apex → 底面边缘
				// Side edge: apex → rim
				lineIndices.push_back(0);
				lineIndices.push_back(rim0);
				// 底面圆周线：底面边缘 → 下一个底面边缘
				// Base circumference: rim → next rim
				lineIndices.push_back(rim0);
				lineIndices.push_back(rim1);
			}
			// 添加静态索引缓冲区
			// Add static index buffer
			AddStaticIndexBuffer(std::make_unique<IndexBuffer>(graphics, lineIndices));

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

			// 设置图元拓扑为线段列表（直接渲染线段，无需线框光栅化）
			// Set primitive topology to line list (render lines directly, no wireframe rasterizer needed)
			AddStaticBind(std::make_unique<Topology>(graphics, D3D11_PRIMITIVE_TOPOLOGY_LINELIST));

			// 实体光栅化状态：LINELIST 拓扑下直接渲染线条，与 DX12 WireframeCone 一致
			// Solid rasterizer: renders lines directly under LINELIST topology, matching DX12 WireframeCone
			AddStaticBind(std::make_unique<Rasterizer>(graphics,
				D3D11_FILL_SOLID, D3D11_CULL_NONE));
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
	void SolidConeDrawable::SetPosition(DirectX::XMFLOAT3 Position) noexcept
	{
		this->Position = Position;
	}

	/** @brief 设置旋转（弧度）
	 *  Set rotation (radians)
	 *
	 *  @param Rotation 旋转角（弧度）/ Rotation angles in radians
	 */
	void SolidConeDrawable::SetRotation(DirectX::XMFLOAT3 Rotation) noexcept
	{
		this->Rotation = Rotation;
	}

	/** @brief 设置颜色
	 *  Set color
	 *
	 *  设置锥体的颜色。
	 *  Sets the color of the cone.
	 *
	 *  @param color 新颜色 / New color
	 */
	void SolidConeDrawable::SetColor(DirectX::XMFLOAT3 color) noexcept
	{
		this->Color = color;
	}

	void SolidConeDrawable::SetScale(DirectX::XMFLOAT3 Scale) noexcept
	{
		this->Scale = Scale;
	}

	/** @brief 更新锥体角度 / Update cone angle
	 *
	 *  重新生成锥体几何体以匹配新的锥角。
	 *  Regenerates cone geometry to match new cone angle.
	 *
	 *  @param height 新高度 / New height
	 *  @param radius 新底面半径 / New base radius
	 */
	void SolidConeDrawable::UpdateAngle(float height, float radius)
	{
		ConeHeight = height;
		ConeRadius = radius;
		RegenerateGeometry();
	}

	/** @brief 重新生成顶点和索引缓冲区 / Regenerate vertex and index buffers
	 *
	 *  根据当前锥体参数重新生成几何体数据并更新 GPU 缓冲区。
	 *  Regenerates geometry data based on current cone parameters and updates GPU buffers.
	 */
	void SolidConeDrawable::RegenerateGeometry()
	{
		// 注意：DX11 的 DrawableBase 不支持动态重新生成静态缓冲区。
		// 对于动态角度变化，需要重新创建 SolidConeDrawable 实例。
		// 这是一个占位方法，为将来可能的动态更新预留接口。
		// Note: DX11's DrawableBase does not support dynamic regeneration of static buffers.
		// For dynamic angle changes, a new SolidConeDrawable instance needs to be created.
		// This is a placeholder method for potential future dynamic updates.
	}

	/** @brief 更新锥体状态
	 *  Update cone state
	 *
	 *  @param dt 时间增量（秒） / Time delta in seconds
	 *  @param aspect 宽高比 / Aspect ratio
	 */
	void SolidConeDrawable::Update(float dt, float aspect) noexcept
	{
		// 锥体无动画更新，保持静态
		// Cone has no animation updates, remains static
	}

	/** @brief 获取变换矩阵
	 *  Get transformation matrix
	 *
	 *  返回锥体的世界变换矩阵。
	 *  Returns the world transformation matrix of the cone.
	 *
	 *  @return DirectX 变换矩阵 / DirectX transformation matrix
	 */
	DirectX::XMMATRIX SolidConeDrawable::GetTransformXM() const noexcept
	{
		// Cone::Generate 生成：apex 在 Z=height，底面在 Z=0
		// 目标：apex 在光源位置（Position），底面沿实际光照方向在前方
		// 注意：shader 中 CalculateSpotLightAttenuation 使用 dot(-vertexToLight, Direction)，
		// 导致实际光照方向与 light.Direction 相反。因此锥体方向需要与 light.Direction 相反，
		// 才能与实际光照方向一致。
		// 变换顺序：缩放 → 平移(-Z)使 apex 到原点 → RotateY(+90°)使底面朝 -X → 光源旋转 → 平移到 Position
		// Cone::Generate: apex at Z=height, base at Z=0
		// Target: apex at light Position, base along actual lighting direction
		// Note: shader's CalculateSpotLightAttenuation uses dot(-vertexToLight, Direction),
		// causing actual lighting direction to be opposite of light.Direction. So the cone
		// must point opposite to light.Direction to match the actual lighting direction.
		// Order: Scale → Translate(-Z) to put apex at origin → RotateY(+90°) to face base toward -X
		//        → light rotation → translate to Position
		return DirectX::XMMatrixScaling(this->Scale.x, this->Scale.y, this->Scale.z) *
			DirectX::XMMatrixTranslation(0.0f, 0.0f, -ConeHeight) *
			DirectX::XMMatrixRotationY(DirectX::XM_PIDIV2) *
			DirectX::XMMatrixRotationRollPitchYaw(
			this->Rotation.x,
			this->Rotation.y,
			this->Rotation.z) *
			DirectX::XMMatrixTranslation(this->Position.x, this->Position.y, this->Position.z);
	}

	/** @brief 获取颜色
	 *  Get color
	 *
	 *  @return 颜色的常量引用 / Const reference to color
	 */
	const XMFLOAT3& SolidConeDrawable::GetColor() const noexcept
	{
		return this->Color;
	}
}