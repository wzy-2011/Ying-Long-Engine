/** @file SolidBoxDrawable.h
 *  @brief 纯色立方体可绘制对象 - Solid-color box drawable object
 *
 *  包含纯色立方体可绘制对象类定义，用于物理驱动的无模型实体占位符。
 *  Contains the solid-color box drawable object class definition, used as
 *  placeholder for physics-driven entities without model.
 */
#pragma once
#include "DrawableBase.h"
#include "../Bindable/InputLayout.h"
#include "../Bindable/TransformConstantBuffer.h"
#include "../Bindable/SolidColorConstantBuffer.h"
#include "../Bindable/VertexShader.h"
#include "../Bindable/PixelShader.h"
#include "../Bindable/Topology.h"
#include "../Bindable/VertexBuffer.h"
#include "../Bindable/IndexBuffer.h"
#include "../Geometry/Cube.h"
#include <DirectXMath.h>

namespace YingLong
{
    /** @brief 纯色立方体可绘制对象类
     *  Solid-color box drawable object class
     *
     *  DX11 纯色立方体占位符，用于没有 MeshComponent::ModelPath 的
     *  物理驱动实体。在 DX12 路径中对应 DX12Box 的角色。
     *  颜色来自 MeshComponent::TintColor（通过 SetColor 设置）；
     *  半尺寸来自 ColliderComponent（烘焙到每个实例的变换中，
     *  而非静态几何体，因此不同大小的多个立方体可以共享单位立方体顶点缓冲区）。
     *
     *  DX11 solid-color box placeholder for physics-driven entities that have no
     *  MeshComponent::ModelPath. Mirrors DX12Box's role in the DX12 path. Color
     *  comes from MeshComponent::TintColor (via SetColor); half-extents come from
     *  ColliderComponent (baked into per-instance transform, not static geometry,
     *  so multiple boxes of different sizes can share the unit-cube vertex buffer).
     */
    class SolidBoxDrawable : public DrawableBase<SolidBoxDrawable>
    {
    public:
        /** @brief 构造函数
         *  Constructor
         *
         *  使用指定的半尺寸和颜色创建立方体。
         *  Creates a box with specified half-extents and color.
         *
         *  @param gfx 图形设备对象引用 / Graphics device object reference
         *  @param halfExtents 半尺寸（每个轴的一半） / Half-extents (half of each axis)
         *  @param color 立方体颜色 / Box color
         */
        SolidBoxDrawable(Graphics& gfx, DirectX::XMFLOAT3 halfExtents, DirectX::XMFLOAT3 color);

        /** @brief 设置位置
         *  Set position
         *
         *  @param Position 新位置 / New position
         */
        void SetPosition(DirectX::XMFLOAT3 Position) noexcept;

        /** @brief 设置颜色
         *  Set color
         *
         *  @param Color 新颜色 / New color
         */
        void SetColor(DirectX::XMFLOAT3 Color) noexcept;

        /** @brief 设置旋转
         *  Set rotation
         *
         *  @param Rotation 新旋转（欧拉角，弧度） / New rotation (Euler angles, radians)
         */
        void SetRotation(DirectX::XMFLOAT3 Rotation) noexcept;

        /** @brief 更新立方体状态
         *  Update box state
         *
         *  物理驱动，无逐帧动画。
         *  Physics-driven, no per-frame animation.
         *
         *  @param dt 时间增量（秒） / Time delta in seconds
         *  @param aspect 宽高比 / Aspect ratio
         */
        void Update(float dt, float aspect) noexcept override;

        /** @brief 获取变换矩阵
         *  Get transformation matrix
         *
         *  返回立方体的世界变换矩阵。
         *  Returns the world transformation matrix of the box.
         *
         *  @return DirectX 变换矩阵 / DirectX transformation matrix
         */
        DirectX::XMMATRIX GetTransformXM() const noexcept override;

        /** @brief 获取颜色
         *  Get color
         *
         *  @return 颜色的常量引用 / Const reference to color
         */
        const DirectX::XMFLOAT3& GetColor() const noexcept;

    private:
        DirectX::XMFLOAT3 Position    = { 0.0f, 0.0f, 0.0f }; ///< 位置 / Position
        DirectX::XMFLOAT3 Rotation    = { 0.0f, 0.0f, 0.0f }; ///< 旋转（欧拉角，弧度） / Rotation (Euler angles, radians)
        DirectX::XMFLOAT3 HalfExtents = { 0.5f, 0.5f, 0.5f }; ///< 半尺寸 / Half-extents
        DirectX::XMFLOAT3 Color        = { 1.0f, 1.0f, 1.0f }; ///< 颜色 / Color
    };
}
