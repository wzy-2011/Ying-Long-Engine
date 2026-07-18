/** @file SolidBoxDrawable.cpp
 *  @brief 纯色立方体可绘制对象实现 - Solid-color box drawable object implementation
 *
 *  包含 SolidBoxDrawable 类的成员函数实现。
 *  Contains the member function implementations of the SolidBoxDrawable class.
 */
#include "SolidBoxDrawable.h"
#include "../Bindable/Rasterizer.h"

namespace YingLong
{
    SolidBoxDrawable::SolidBoxDrawable(Graphics& graphics, DirectX::XMFLOAT3 halfExtents, DirectX::XMFLOAT3 color)
        : HalfExtents(halfExtents), Color(color)
    {
        if (!IsStaticInitialized())
        {
            struct Vertex
            {
                DirectX::XMFLOAT3 Position;
                DirectX::XMFLOAT3 Color;
            };
            auto Model = Cube::Make<Vertex>();
            for (auto& v : Model.vertices)
            {
                v.Color = { 1.0f, 1.0f, 1.0f };
            }
            AddStaticBind(std::make_unique<VertexBuffer>(graphics, Model.vertices));
            AddStaticIndexBuffer(std::make_unique<IndexBuffer>(graphics, Model.indices));

            auto pVertexShader = std::make_unique<VertexShader>(graphics,
                "CodeFile/Shader/SolidVertexShader.hlsl");
            auto pVertexShaderByteCode = pVertexShader->GetBytecode();
            AddStaticBind(std::move(pVertexShader));

            AddStaticBind(std::make_unique<PixelShader>(graphics,
                "CodeFile/Shader/SolidPixelShader.hlsl"));

            const std::vector<D3D11_INPUT_ELEMENT_DESC> ied =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };
            AddStaticBind(std::make_unique<InputLayout>(graphics, ied, pVertexShaderByteCode));

            AddStaticBind(std::make_unique<Topology>(graphics, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST));

            AddStaticBind(std::make_unique<Rasterizer>(graphics,
                D3D11_FILL_SOLID, D3D11_CULL_BACK));
        }
        else
        {
            FindIndexBufferFromStatic();
        }

        AddBind(std::make_unique<TransformConstantBuffer>(graphics, *this));
        AddBind(std::make_unique<SolidColorConstantBuffer>(graphics, &GetColor()));
    }

    void SolidBoxDrawable::SetPosition(DirectX::XMFLOAT3 Position) noexcept
    {
        this->Position = Position;
    }

    void SolidBoxDrawable::SetColor(DirectX::XMFLOAT3 Color) noexcept
    {
        this->Color = Color;
    }

    void SolidBoxDrawable::SetRotation(DirectX::XMFLOAT3 Rotation) noexcept
    {
        this->Rotation = Rotation;
    }

    void SolidBoxDrawable::Update(float dt, float aspect) noexcept
    {
    }

    DirectX::XMMATRIX SolidBoxDrawable::GetTransformXM() const noexcept
    {
        namespace dx = DirectX;
        return dx::XMMatrixScaling(2.0f * HalfExtents.x, 2.0f * HalfExtents.y, 2.0f * HalfExtents.z)
             * dx::XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z)
             * dx::XMMatrixTranslation(Position.x, Position.y, Position.z);
    }

    const DirectX::XMFLOAT3& SolidBoxDrawable::GetColor() const noexcept
    {
        return this->Color;
    }
}
