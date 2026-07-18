/**
 * @file VertexBuffer.h
 * @brief DX11 顶点缓冲区头文件 / DX11 vertex buffer header file
 *
 * 封装 D3D11 顶点缓冲区模板类，用于存储几何体顶点数据并绑定到输入装配阶段。
 * Encapsulates D3D11 vertex buffer template class for storing geometry
 * vertex data and binding to the input assembler stage.
 */

#pragma once
#include "Bindable.h"
#include "../Graphics.h"
#include <vector>

namespace YingLong
{
    /**
     * @brief DX11 顶点缓冲区类 / DX11 vertex buffer class
     *
     * 管理 D3D11 顶点缓冲区资源，支持从任意顶点类型的向量创建
     * 顶点缓冲区，并绑定到输入装配阶段。顶点布局由模板参数 V 决定。
     *
     * Manages D3D11 vertex buffer resources, supporting creation of vertex
     * buffers from vectors of any vertex type and binding to the input
     * assembler stage. The vertex layout is determined by template parameter V.
     *
     * @tparam V 顶点数据结构类型 / Vertex data structure type
     */
    class VertexBuffer : public Bindable
    {
    public:
        UINT stride;   ///< 单个顶点的字节跨度 / Byte stride of a single vertex

        /**
         * @brief 构造函数 / Constructor
         * @tparam V 顶点数据结构类型 / Vertex data structure type
         * @param graphics 图形设备引用 / Graphics device reference
         * @param vertices 顶点数据向量 / Vertex data vector
         *
         * 根据传入的顶点数据向量创建 D3D11 顶点缓冲区，
         * 自动计算顶点跨度和缓冲区大小。
         * Creates a D3D11 vertex buffer from the provided vertex data vector,
         * automatically calculating vertex stride and buffer size.
         */
        template<class V>
        VertexBuffer(Graphics& graphics, const std::vector<V>& vertices) : stride(sizeof(V))
        {
            HRESULT hr;
            D3D11_BUFFER_DESC VerteBufferDesc = { 0 };
            // 设置为顶点缓冲区绑定类型
            // Set as vertex buffer bind type
            VerteBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            // 默认使用方式，GPU 读写，CPU 不可访问
            // Default usage, GPU read-write, CPU not accessible
            VerteBufferDesc.Usage = D3D11_USAGE_DEFAULT;
            VerteBufferDesc.CPUAccessFlags = 0u;
            VerteBufferDesc.MiscFlags = 0u;
            VerteBufferDesc.ByteWidth = UINT(sizeof(V) * vertices.size());
            VerteBufferDesc.StructureByteStride = stride;

            D3D11_SUBRESOURCE_DATA subresource = { 0 };
            subresource.pSysMem = vertices.data();
            // 创建 D3D11 顶点缓冲区资源
            // Create D3D11 vertex buffer resource
            GRAPHICS_THROW_EXCEPTION(GetDevice(&graphics)->CreateBuffer(&VerteBufferDesc, 
                &subresource, &pVertexBuffer));
        }

        /**
         * @brief 绑定顶点缓冲区 / Bind vertex buffer
         * @param graphics 图形设备引用 / Graphics device reference
         *
         * 将顶点缓冲区绑定到输入装配阶段的第 0 个槽位。
         * Binds the vertex buffer to slot 0 of the input assembler stage.
         */
        void Bind(Graphics& graphics) noexcept override
        {
            UINT offset = 0u;
            GetDevicContext(&graphics)->IASetVertexBuffers(0u, 1u, pVertexBuffer.GetAddressOf(), &stride, &offset);
        }

    protected:
        WRL::ComPtr<ID3D11Buffer> pVertexBuffer;   ///< D3D11 顶点缓冲区对象 / D3D11 vertex buffer object
    };
}
