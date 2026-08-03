/** @file Capsule.cpp
 *  @brief 胶囊体可绘制对象实现 - Capsule drawable object implementation
 *
 *  包含 CapsuleDrawable 类的成员函数实现。
 *  Contains the member function implementations of the CapsuleDrawable class.
 */
#include "Capsule.h"
#include "../Bindable/VertexBuffer.h"
#include "../Bindable/VertexShader.h"
#include "../Bindable/PixelShader.h"
#include "../Bindable/InputLayout.h"
#include "../Bindable/Topology.h"
#include "../Bindable/Texture.h"
#include "../Bindable/Rasterizer.h"
#include "../Bindable/Sampler.h"
#include "../../ImGui/CodeFile/ImGui/imgui.h"

namespace YingLong
{
	/** @brief 构造函数
	 *  Constructor
	 *
	 *  使用指定的半径、半高和颜色创建胶囊体。
	 *  Creates a capsule with specified radius, half height, and color.
	 *
	 *  @param graphics 图形设备对象引用 / Graphics device object reference
	 *  @param radius 胶囊体半径 / Capsule radius
	 *  @param HalfHeight 胶囊体半高 / Capsule half height
	 *  @param Color 胶囊体颜色 / Capsule color
	 */
	CapsuleDrawable::CapsuleDrawable(Graphics& graphics, float radius,
		float HalfHeight, XMFLOAT3 Color)
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
			// 生成带蒙皮的胶囊体模型
			// Generate skinned capsule model
			auto Model = Capsule::MakeSkinned<Vertex>(radius, HalfHeight);
			// 设置独立平面法线
			// Set independent flat normals
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

			// 实体光栅化状态：确保胶囊体以实体模式渲染
			// Solid rasterizer state: ensure capsule renders as solid
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
			std::make_unique<Texture>(graphics, Surface("Resources/Pictures/Direct3D.png"));
		AddBind(std::move(texture));

		// 添加变换常量缓冲区
		// Add transform constant buffer
		AddBind(std::make_unique<TransformConstantBuffer>(graphics, *this));
	}

	/** @brief 拷贝构造函数
	 *  Copy constructor
	 *
	 *  @param other 要拷贝的胶囊体对象 / The capsule object to copy from
	 */
	CapsuleDrawable::CapsuleDrawable(const CapsuleDrawable& other) noexcept
	{
		// 拷贝所有属性
		// Copy all properties
		this->Color = other.Color;
		this->Position = other.Position;
		this->radius = other.radius;
		this->HalfHeight = other.HalfHeight;
		this->Rotation = other.Rotation;
	}

	/** @brief 更新胶囊体状态
	 *  Update capsule state
	 *
	 *  @param dt 时间增量（秒） / Time delta in seconds
	 *  @param aspect 宽高比 / Aspect ratio
	 */
	void CapsuleDrawable::Update(float dt, float aspect) noexcept
	{

	}

	/** @brief 创建 ImGui 控制窗口
	 *  Spawn ImGui control window
	 *
	 *  创建用于调整胶囊体参数的 ImGui 窗口。
	 *  Creates an ImGui window for adjusting capsule parameters.
	 *
	 *  @param ImGuiWindowName ImGui 窗口名称 / ImGui window name
	 */
	void CapsuleDrawable::SpawnControlWindow(const char* ImGuiWindowName) noexcept
	{
		// 将位置和旋转转换为数组供 ImGui 使用
		// Convert position and rotation to arrays for ImGui
		float position[3] = {
			this->Position.x,
			this->Position.y,
			this->Position.z
		};

		float rotation[3] = {
			this->Rotation.x,
			this->Rotation.y,
			this->Rotation.z
		};

		// 开始 ImGui 窗口
		// Begin ImGui window
		ImGui::Begin(ImGuiWindowName);

		// 位置控制
		// Position control
		ImGui::Text("位置");
		if (ImGui::DragFloat3("位置", position, 0.1f))
		{
			this->Position = { position[0], position[1], position[2] };
		}

		// 旋转控制
		// Rotation control
		ImGui::Text("旋转");
		if (ImGui::DragFloat3("旋转", rotation, 1.0f))
		{
			this->Rotation = { rotation[0], rotation[1], rotation[2] };
		}

		// 半径控制
		// Radius control
		ImGui::Text("半径");
		ImGui::DragFloat("半径", &this->radius, 0.08f);

		// 半高控制
		// Half height control
		ImGui::Text("半高");
		ImGui::DragFloat("半高", &this->HalfHeight, 0.08f);

		// 结束 ImGui 窗口
		// End ImGui window
		ImGui::End();
	}

	/** @brief 获取变换矩阵
	 *  Get transformation matrix
	 *
	 *  返回胶囊体的世界变换矩阵。
	 *  Returns the world transformation matrix of the capsule.
	 *
	 *  @return DirectX 变换矩阵 / DirectX transformation matrix
	 */
	XMMATRIX CapsuleDrawable::GetTransformXM() const noexcept
	{
		// 变换顺序：旋转 -> 平移
		// Transformation order: rotation -> translation
		return XMMatrixRotationRollPitchYaw(
			XMConvertToRadians(Rotation.x),
			XMConvertToRadians(Rotation.y),
			XMConvertToRadians(Rotation.z)
		) * XMMatrixTranslation(Position.x, Position.y, Position.z);
	}
}
