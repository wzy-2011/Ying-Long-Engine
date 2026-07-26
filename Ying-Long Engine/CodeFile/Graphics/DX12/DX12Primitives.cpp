/**
 * @file DX12Primitives.cpp
 * @brief DX12 几何图元实现文件 / DX12 Geometric Primitives Implementation
 *
 * 本文件实现了 DX12Primitive 基类及其派生类 DX12Triangle 和 DX12Box，
 * 包括几何体创建、材质初始化、变换更新、绘制等功能。
 *
 * This file implements the DX12Primitive base class and its derived classes
 * DX12Triangle and DX12Box, including geometry creation, material initialization,
 * transform update, drawing, etc.
 */

#include "DX12Primitives.h"
#include <DirectXMath.h>
#include <cmath>
#include <cassert>

using namespace DirectX;

namespace YingLong
{
    // =============================================================================
    // DX12Primitive base implementation
    // DX12Primitive 基类实现
    // =============================================================================

    /**
     * @brief 构造函数实现 / Constructor implementation
     *
     * 只初始化单位矩阵。不能在这里调用 CreateGeometry()/InitMaterial()，
     * 因为在基类构造期间，虚函数派发不会到达派生类的重写版本。
     * 派生类构造函数改为调用 Initialize()。
     *
     * Only initializes identity matrices. CreateGeometry()/InitMaterial() cannot
     * be called here because virtual dispatch does not reach derived overrides
     * during base construction. Derived constructors call Initialize() instead.
     *
     * @param core DX12 核心对象引用 / DX12 core object reference
     */
    DX12Primitive::DX12Primitive(DX12Core& core)
        : Core(core)  ///< DX12 核心引用 / DX12 core reference
    {
        // 初始化视图矩阵为单位矩阵
        // Initialize view matrix as identity
        ViewMatrix[0] = ViewMatrix[5] = ViewMatrix[10] = ViewMatrix[15] = 1.0f;
        // 初始化投影矩阵为单位矩阵
        // Initialize projection matrix as identity
        ProjectionMatrix[0] = ProjectionMatrix[5] = ProjectionMatrix[10] = ProjectionMatrix[15] = 1.0f;
    }

    /**
     * @brief 初始化图元 / Initialize primitive
     *
     * 调用派生类的 CreateGeometry() 创建几何体，
     * 然后创建变换缓冲区和占位符常量缓冲区。
     *
     * Calls the derived class's CreateGeometry() to create geometry,
     * then creates the transform buffer and placeholder constant buffers.
     */
    void DX12Primitive::Initialize()
    {
        // 创建几何体（虚函数，由派生类实现）
        // Create geometry (virtual function, implemented by derived class)
        CreateGeometry();

        // 创建变换常量缓冲区
        // Create transform constant buffer
        CreateTransformBuffer();

        // 创建占位符常量缓冲区（点光源、聚光源、材质）
        // Create placeholder constant buffers (point light, spot light, material)
        CreatePlaceholderCBVs();
    }

    /**
     * @brief 创建变换缓冲区 / Create transform buffer
     *
     * 创建包含 3 个元素的变换常量缓冲区（双缓冲 + 1），
     * 初始化为单位矩阵。
     *
     * Creates a transform constant buffer with 3 elements (double buffer + 1),
     * initialized to identity matrix.
     */
    void DX12Primitive::CreateTransformBuffer()
    {
        // 初始化为单位矩阵
        // Initialize as identity matrix
        DX12Transform transform = {};
        transform.ModelMatrix[0] = transform.ModelMatrix[5] = transform.ModelMatrix[10] = transform.ModelMatrix[15] = 1.0f;
        transform.ModelViewProjMatrix[0] = transform.ModelViewProjMatrix[5] = transform.ModelViewProjMatrix[10] = transform.ModelViewProjMatrix[15] = 1.0f;

        // 创建变换常量缓冲区（3个元素用于双缓冲）
        // Create transform constant buffer (3 elements for double buffering)
        pTransformBuffer = std::make_unique<ConstantBufferDX12<DX12Transform>>(Core, 2, transform);
    }

    /**
     * @brief 创建占位符常量缓冲区 / Create placeholder constant buffers
     *
     * 创建点光源、聚光源和材质的占位符常量缓冲区。
     * 点光源和聚光源初始化为 0 个光源，材质由派生类初始化。
     *
     * Creates placeholder constant buffers for point light, spot light, and material.
     * Point light and spot light are initialized with 0 lights, material is
     * initialized by derived class.
     */
    void DX12Primitive::CreatePlaceholderCBVs()
    {
        // 光源计数 CB：0 个光源
        // Light count CB: 0 lights
        DX12LightCountCB lightCountData = {};
        lightCountData.PointLightCount = 0;
        lightCountData.SpotLightCount = 0;
        pLightCountBuffer = std::make_unique<ConstantBufferDX12<DX12LightCountCB>>(Core, 0, lightCountData);

        // 材质 CB：由派生类填充反照率/金属度/粗糙度/...
        // Material CB: derived class fills in albedo/metallic/roughness/...
        DX12MaterialCB materialData = {};
        InitMaterial(materialData);
        pMaterialBuffer = std::make_unique<ConstantBufferDX12<DX12MaterialCB>>(Core, 1, materialData);
    }

    /**
     * @brief 更新图元状态 / Update primitive state
     *
     * 根据增量时间更新旋转角度，然后更新变换缓冲区。
     * Updates rotation angles based on delta time, then updates the transform buffer.
     *
     * @param deltaTime 增量时间（秒）/ Delta time in seconds
     */
    void DX12Primitive::Update(float deltaTime)
    {
        // 根据旋转速度和增量时间更新旋转角度
        // Update rotation angles based on rotation speed and delta time
        Rotation[0] += deltaTime * RotationSpeed[0];
        Rotation[1] += deltaTime * RotationSpeed[1];
        Rotation[2] += deltaTime * RotationSpeed[2];

        // 更新变换缓冲区
        // Update transform buffer
        UpdateTransformBuffer();
    }

    /**
     * @brief 更新变换缓冲区 / Update transform buffer
     *
     * 根据当前位置、旋转、缩放以及视图和投影矩阵重新计算
     * 模型矩阵和 MVP 矩阵，并更新到常量缓冲区。
     *
     * 注意：HLSL 期望列主序矩阵，而 DirectXMath 使用行主序，
     * 因此需要转置。
     *
     * Recalculates model matrix and MVP matrix based on current position,
     * rotation, scale, and view and projection matrices, and updates them
     * to the constant buffer.
     *
     * Note: HLSL expects column-major matrices, while DirectXMath uses
     * row-major, so transposition is needed.
     */
    void DX12Primitive::UpdateTransformBuffer()
    {
        XMMATRIX modelMatrix = XMMatrixIdentity();

        // 正确的变换顺序：缩放 * 旋转 * 平移
        // 因为 HLSL 中 mul(v, M) 表示 v * M，向量在左边
        // Correct transform order: scale * rotation * translation
        // Because in HLSL mul(v, M) means v * M, vector is on the left
        modelMatrix *= XMMatrixScaling(Scale, Scale, Scale);
        modelMatrix *= XMMatrixRotationRollPitchYaw(Rotation[0], Rotation[1], Rotation[2]);
        modelMatrix *= XMMatrixTranslation(Position[0], Position[1], Position[2]);

        // 加载视图矩阵和投影矩阵
        // Load view matrix and projection matrix
        XMMATRIX viewMatrix = XMLoadFloat4x4(reinterpret_cast<XMFLOAT4X4*>(ViewMatrix));
        XMMATRIX projMatrix = XMLoadFloat4x4(reinterpret_cast<XMFLOAT4X4*>(ProjectionMatrix));

        // 计算 MVP 矩阵：模型 * 视图 * 投影
        // Calculate MVP matrix: model * view * projection
        XMMATRIX mvpMatrix = modelMatrix * viewMatrix * projMatrix;

        DX12Transform transform = {};

        // HLSL 期望列主序矩阵，DirectXMath 使用行主序，因此需要转置
        // HLSL expects column-major matrices, DirectXMath uses row-major, so transpose
        XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(transform.ModelMatrix), XMMatrixTranspose(modelMatrix));
        XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(transform.ModelViewProjMatrix), XMMatrixTranspose(mvpMatrix));

        // 更新变换常量缓冲区
        // Update transform constant buffer
        pTransformBuffer->Update(transform);
    }

    /**
     * @brief 绘制图元 / Draw primitive
     *
     * 将根签名、PSO、所有常量缓冲区、描述符表、顶点缓冲区、
     * 索引缓冲区绑定到命令列表，然后执行绘制调用。
     *
     * Binds root signature, PSO, all constant buffers, descriptor tables,
     * vertex buffer, and index buffer to the command list, then executes
     * the draw call.
     *
     * @param commandList 图形命令列表指针 / Graphics command list pointer
     */
    void DX12Primitive::Draw(ID3D12GraphicsCommandList* commandList)
    {
        // 绑定根签名
        // Bind root signature
        if (Core.GetRootSignature() && Core.GetRootSignature()->GetRootSignature())
        {
            commandList->SetGraphicsRootSignature(Core.GetRootSignature()->GetRootSignature());
        }

        // 绑定管线状态对象
        // Bind pipeline state object
        if (Core.GetPipelineState() && Core.GetPipelineState()->IsInitialized())
        {
            Core.GetPipelineState()->Bind(commandList);
        }

        // 绑定所有根参数（0-2：CBV）
        // Bind all root parameters (0-2: CBVs)
        if (pLightCountBuffer)
            pLightCountBuffer->Bind(commandList);
        if (pMaterialBuffer)
            pMaterialBuffer->Bind(commandList);
        if (pTransformBuffer)
            pTransformBuffer->Bind(commandList);

        // 根参数 3：纹理描述符表（t0-t3）
        // Root param 3: Texture descriptor table (t0-t3)
        DX12DescriptorHeap* cbvSrvHeap = Core.GetCBVSRVUAVHeap();
        DX12DescriptorHeap* samplerHeap = Core.GetSamplerHeap();
        if (cbvSrvHeap)
        {
            commandList->SetGraphicsRootDescriptorTable(3, cbvSrvHeap->GetGPUHandle(0));
        }

        // 根参数 4：光源缓冲区描述符表（t4-t5）
        // Root param 4: Light buffer descriptor table (t4-t5)
        if (cbvSrvHeap && PointLightBuffer.GetSRVIndex() != UINT_MAX)
        {
            commandList->SetGraphicsRootDescriptorTable(4, cbvSrvHeap->GetGPUHandle(PointLightBuffer.GetSRVIndex()));
        }

        // 根参数 5：采样器描述符表
        // Root param 5: Sampler descriptor table
        if (samplerHeap)
        {
            commandList->SetGraphicsRootDescriptorTable(5, samplerHeap->GetGPUHandle(0));
        }

        // 绑定顶点缓冲区
        // Bind vertex buffer
        if (pVertexBuffer)
            pVertexBuffer->Bind(commandList);

        // 绑定索引缓冲区
        // Bind index buffer
        if (pIndexBuffer)
            pIndexBuffer->Bind(commandList);

        // 设置图元拓扑为三角形列表
        // Set primitive topology to triangle list
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // 执行绘制调用（索引绘制，1个实例）
        // Execute draw call (indexed draw, 1 instance)
        commandList->DrawIndexedInstanced(IndexCount, 1, 0, 0, 0);
    }

    /**
     * @brief 设置位置 / Set position
     * @param x X 坐标 / X coordinate
     * @param y Y 坐标 / Y coordinate
     * @param z Z 坐标 / Z coordinate
     */
    void DX12Primitive::SetPosition(float x, float y, float z)
    {
        Position[0] = x; Position[1] = y; Position[2] = z;
        UpdateTransformBuffer();
    }

    /**
     * @brief 设置旋转 / Set rotation
     * @param pitch 俯仰角（弧度）/ Pitch angle in radians
     * @param yaw 偏航角（弧度）/ Yaw angle in radians
     * @param roll 滚转角（弧度）/ Roll angle in radians
     */
    void DX12Primitive::SetRotation(float pitch, float yaw, float roll)
    {
        Rotation[0] = pitch; Rotation[1] = yaw; Rotation[2] = roll;
        UpdateTransformBuffer();
    }

    /**
     * @brief 设置缩放 / Set scale
     * @param scale 统一缩放值 / Uniform scale value
     */
    void DX12Primitive::SetScale(float scale)
    {
        Scale = scale;
        UpdateTransformBuffer();
    }

    /**
     * @brief 重新创建顶点和索引缓冲区 / Recreate vertex and index buffers
     *
     * 允许在初始化后动态更新几何体数据。
     * 用于锥体线框在锥角变化时的几何体更新。
     *
     * Allows dynamic geometry updates after initialization.
     * Used for cone wireframe geometry updates when cone angle changes.
     *
     * @param vertices 新顶点数据 / New vertex data
     * @param indices 新索引数据 / New index data
     * @param indexCount 索引数量 / Index count
     */
    void DX12Primitive::RecreateGeometry(const std::vector<DX12Vertex>& vertices,
        const std::vector<UINT>& indices, UINT indexCount)
    {
        IndexCount = indexCount;
        pVertexBuffer = std::make_unique<VertexBufferDX12<DX12Vertex>>(Core, vertices);
        pIndexBuffer = std::make_unique<IndexBufferDX12<UINT>>(Core, indices);
    }

    /**
     * @brief 设置视图矩阵 / Set view matrix
     * @param viewMatrix 视图矩阵数组（16个元素）/ View matrix array (16 elements)
     */
    void DX12Primitive::SetViewMatrix(const float* viewMatrix)
    {
        if (viewMatrix)
            memcpy(ViewMatrix, viewMatrix, sizeof(ViewMatrix));
    }

    /**
     * @brief 设置投影矩阵 / Set projection matrix
     * @param projMatrix 投影矩阵数组（16个元素）/ Projection matrix array (16 elements)
     */
    void DX12Primitive::SetProjectionMatrix(const float* projMatrix)
    {
        if (projMatrix)
            memcpy(ProjectionMatrix, projMatrix, sizeof(ProjectionMatrix));
    }

    /**
     * @brief 设置光源计数和相机位置数据 / Set light count and camera position data
     * @param data 光源计数常量缓冲区数据 / Light count constant buffer data
     */
    void DX12Primitive::SetLightCountData(const DX12LightCountCB& data)
    {
        if (pLightCountBuffer)
            pLightCountBuffer->Update(data);
    }

    /**
     * @brief 初始化静态光源缓冲区 / Initialize static light buffers
     * @param core DX12 核心对象引用 / DX12 core object reference
     * @param initialCapacity 初始容量 / Initial capacity
     */
    void DX12Primitive::InitializeLightBuffers(DX12Core& core, UINT initialCapacity)
    {
        // 根参数 4 是一个包含 2 个 SRV（t4-t5）的描述符表，
        // 要求 PointLightBuffer 和 SpotLightBuffer 的 SRV 在堆中连续。
        // 当 FreeList 为空时，两次 Allocate() 返回连续索引。
        // 此方法应在初始化早期调用（在描述符释放之前）以确保连续性。
        // Root parameter 4 is a descriptor table with 2 SRVs (t4-t5),
        // requiring PointLightBuffer and SpotLightBuffer SRVs to be
        // consecutive in the heap. When FreeList is empty, two Allocate()
        // calls return consecutive indices. This method should be called
        // early in initialization (before any descriptor frees) to ensure
        // contiguity.
        UINT pointLightSRVIndex = core.GetCBVSRVUAVHeap()->Allocate();
        UINT spotLightSRVIndex = core.GetCBVSRVUAVHeap()->Allocate();

        // 验证索引连续性（仅在调试构建中检查）
        // Verify index contiguity (checked only in debug builds)
        assert(spotLightSRVIndex == pointLightSRVIndex + 1 &&
            "Light buffer SRV indices must be consecutive for descriptor table!");

        PointLightBuffer.InitializeWithSRVIndex(core, initialCapacity, pointLightSRVIndex);
        SpotLightBuffer.InitializeWithSRVIndex(core, initialCapacity, spotLightSRVIndex);
    }

    /**
     * @brief 更新点光源结构化缓冲区 / Update point light structured buffer
     * @param data 点光源数据向量 / Point light data vector
     */
    void DX12Primitive::UpdatePointLightBuffer(const std::vector<DX12PointLightData>& data)
    {
        PointLightBuffer.Update(data);
    }

    /**
     * @brief 更新聚光源结构化缓冲区 / Update spot light structured buffer
     * @param data 聚光源数据向量 / Spot light data vector
     */
    void DX12Primitive::UpdateSpotLightBuffer(const std::vector<DX12SpotLightData>& data)
    {
        SpotLightBuffer.Update(data);
    }

    /**
     * @brief 清理静态光源缓冲区资源 / Clean up static light buffer resources
     */
    void DX12Primitive::CleanupLightBuffers()
    {
        PointLightBuffer.ReleaseResources();
        SpotLightBuffer.ReleaseResources();
    }

    /**
     * @brief 更新光源缓冲区（每帧调用一次） / Update light buffers (called once per frame)
     */
    void DX12Primitive::UpdateLightBuffers(ID3D12GraphicsCommandList* commandList)
    {
        PointLightBuffer.ApplyUpdate(commandList);
        SpotLightBuffer.ApplyUpdate(commandList);
    }

    // 静态成员变量初始化
    // Static member variable initialization
    StructuredBufferDX12<DX12PointLightData> DX12Primitive::PointLightBuffer;
    StructuredBufferDX12<DX12SpotLightData> DX12Primitive::SpotLightBuffer;

    // =============================================================================
    // DX12Triangle
    // DX12 三角形
    // =============================================================================

    /**
     * @brief 创建三角形几何体 / Create triangle geometry
     *
     * 创建一个包含 3 个顶点的三角形，法线指向 +Z 方向。
     * 默认绕 Z 轴旋转。
     *
     * Creates a triangle with 3 vertices, normal pointing in +Z direction.
     * Default rotation around Z axis.
     */
    void DX12Triangle::CreateGeometry()
    {
        // 定义三角形顶点（位置、法线、纹理坐标）
        // Define triangle vertices (position, normal, texture coordinate)
        std::vector<DX12Vertex> vertices = {
            { { 0.0f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.5f, 0.0f } },   ///< 顶点0：顶部 / Vertex 0: top
            { { 0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },   ///< 顶点1：右下 / Vertex 1: bottom right
            { {-0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },   ///< 顶点2：左下 / Vertex 2: bottom left
        };

        // 定义索引
        // Define indices
        std::vector<UINT> indices = { 0, 1, 2 };

        IndexCount = static_cast<UINT>(indices.size());

        // 创建顶点缓冲区和索引缓冲区
        // Create vertex buffer and index buffer
        pVertexBuffer = std::make_unique<VertexBufferDX12<DX12Vertex>>(Core, vertices);
        pIndexBuffer = std::make_unique<IndexBufferDX12<UINT>>(Core, indices);

        // 默认绕 Z 轴旋转
        // Default per-instance rotation around Z
        RotationSpeed[2] = 0.5f;
    }

    /**
     * @brief 初始化三角形材质 / Initialize triangle material
     *
     * 初始化为灰色、非金属、中等粗糙度、无纹理。
     * Initializes to gray, non-metallic, medium roughness, no textures.
     *
     * @param materialData 材质数据引用 / Material data reference
     */
    void DX12Triangle::InitMaterial(DX12MaterialCB& materialData)
    {
        materialData.Albedo[0] = 0.6f;              ///< 反照率 R / Albedo R
        materialData.Albedo[1] = 0.6f;              ///< 反照率 G / Albedo G
        materialData.Albedo[2] = 0.6f;              ///< 反照率 B / Albedo B
        materialData.Metallic = 0.0f;                ///< 金属度 / Metallic
        materialData.Roughness = 0.5f;               ///< 粗糙度 / Roughness
        materialData.AmbientOcclusion = 1.0f;        ///< 环境光遮蔽 / Ambient occlusion
        materialData.UseAlbedoTexture = 0;           ///< 不使用反照率纹理 / No albedo texture
        materialData.UseRoughnessTexture = 0;        ///< 不使用粗糙度纹理 / No roughness texture
        materialData.UseMetallicTexture = 0;         ///< 不使用金属度纹理 / No metallic texture
        materialData.UseNormalTexture = 0;           ///< 不使用法线纹理 / No normal texture
        materialData.UseAOTexture = 0;               ///< 不使用 AO 纹理 / No AO texture
        materialData.Unlit = 0;                     ///< 参与光照计算 / Participates in lighting
    }
    // DX12Box
    // DX12 立方体
    // =============================================================================

    /**
     * @brief 创建立方体几何体 / Create box geometry
     *
     * 创建一个包含 24 个顶点（6 个面，每面 4 个顶点）和
     * 36 个索引的立方体。默认绕 X 和 Y 轴旋转。
     *
     * Creates a box with 24 vertices (6 faces, 4 vertices per face) and
     * 36 indices. Default rotation around X and Y axes.
     */
    void DX12Box::CreateGeometry()
    {
        // 定义立方体顶点（6个面，每面4个顶点）
        // Define box vertices (6 faces, 4 vertices per face)
        std::vector<DX12Vertex> vertices = {
            // 前面 / Front face
            { {-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f} },
            { { 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} },
            { { 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f} },
            { {-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} },
            // 后面 / Back face
            { {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f} },
            { {-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f} },
            { { 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f} },
            { { 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f} },
            // 顶面 / Top face
            { {-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} },
            { {-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f} },
            { { 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} },
            { { 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f} },
            // 底面 / Bottom face
            { {-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f} },
            { { 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f} },
            { { 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f} },
            { {-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f} },
            // 右面 / Right face
            { { 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} },
            { { 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} },
            { { 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} },
            { { 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} },
            // 左面 / Left face
            { {-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} },
            { {-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} },
            { {-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} },
            { {-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} },
        };

        // 定义索引（6个面，每面2个三角形 = 36个索引）
        // Define indices (6 faces, 2 triangles per face = 36 indices)
        std::vector<UINT> indices = {
            // 前面 / Front face
            0, 1, 2, 0, 2, 3,
            // 后面 / Back face
            4, 5, 6, 4, 6, 7,
            // 顶面 / Top face
            8, 9, 10, 8, 10, 11,
            // 底面 / Bottom face
            12, 13, 14, 12, 14, 15,
            // 右面 / Right face
            16, 17, 18, 16, 18, 19,
            // 左面 / Left face
            20, 21, 22, 20, 22, 23,
        };

        IndexCount = static_cast<UINT>(indices.size());

        // 创建顶点缓冲区和索引缓冲区
        // Create vertex buffer and index buffer
        pVertexBuffer = std::make_unique<VertexBufferDX12<DX12Vertex>>(Core, vertices);
        pIndexBuffer = std::make_unique<IndexBufferDX12<UINT>>(Core, indices);

        // 默认绕 X 和 Y 轴旋转
        // Default per-instance rotation around X and Y
        RotationSpeed[0] = 0.3f;
        RotationSpeed[1] = 0.5f;
    }

    /**
     * @brief 初始化立方体材质 / Initialize box material
     *
     * 使用 Color 成员变量作为反照率颜色，其余参数为默认值。
     * Uses the Color member variable as albedo color, with default values for rest.
     *
     * @param materialData 材质数据引用 / Material data reference
     */
    void DX12Box::InitMaterial(DX12MaterialCB& materialData)
    {
        materialData.Albedo[0] = Color[0];         ///< 反照率 R / Albedo R
        materialData.Albedo[1] = Color[1];         ///< 反照率 G / Albedo G
        materialData.Albedo[2] = Color[2];         ///< 反照率 B / Albedo B
        materialData.Metallic = 0.0f;               ///< 金属度 / Metallic
        materialData.Roughness = 0.5f;              ///< 粗糙度 / Roughness
        materialData.AmbientOcclusion = 1.0f;       ///< 环境光遮蔽 / Ambient occlusion
        materialData.UseAlbedoTexture = 0;          ///< 不使用反照率纹理 / No albedo texture
        materialData.UseRoughnessTexture = 0;       ///< 不使用粗糙度纹理 / No roughness texture
        materialData.UseMetallicTexture = 0;        ///< 不使用金属度纹理 / No metallic texture
        materialData.UseNormalTexture = 0;          ///< 不使用法线纹理 / No normal texture
        materialData.UseAOTexture = 0;          ///< 不使用 AO 纹理 / No AO texture
        materialData.Unlit = 0;                ///< 参与光照计算 / Participates in lighting
    }

    /**
     * @brief 设置颜色 / Set color
     *
     * 设置立方体的颜色，并更新材质常量缓冲区。
     * Sets the box's color and updates the material constant buffer.
     *
     * @param r 红色分量 / Red component
     * @param g 绿色分量 / Green component
     * @param b 蓝色分量 / Blue component
     * @param a Alpha 分量 / Alpha component
     */
    void DX12Box::SetColor(float r, float g, float b, float a)
    {
        Color[0] = r; Color[1] = g; Color[2] = b; Color[3] = a;

        // 使用新的反照率重建材质 CB
        // Rebuild material CB with new albedo
        if (pMaterialBuffer)
        {
            DX12MaterialCB materialData = {};
            InitMaterial(materialData);
            pMaterialBuffer->Update(materialData);
        }
    }

    // =============================================================================
    // DX12Sphere
    // DX12 球体（用于点光源可视化）
    // =============================================================================

    /**
     * @brief 创建球体几何体 / Create sphere geometry
     *
     * 使用经纬度细分算法生成球体网格，半径固定为 0.3 单位。
     * 分段数 16×16，在视觉效果和性能之间取得平衡（60fps+）。
     *
     * Generates sphere mesh using latitude/longitude subdivision algorithm,
     * with a fixed radius of 0.3 units. Uses 16×16 segments for a balance
     * between visual quality and performance (60fps+).
     */
    void DX12Sphere::CreateGeometry()
    {
        const UINT segments = 16;
        const UINT rings = 16;
        const float radius = 0.3f;

        std::vector<DX12Vertex> vertices;
        std::vector<UINT> indices;

        // 生成顶点 / Generate vertices
        for (UINT ring = 0; ring <= rings; ++ring)
        {
            float latitude = XM_PIDIV2 - static_cast<float>(ring) * XM_PI / rings;
            float z = radius * sinf(latitude);
            float r = radius * cosf(latitude);

            for (UINT segment = 0; segment <= segments; ++segment)
            {
                float longitude = static_cast<float>(segment) * XM_2PI / segments;

                DX12Vertex vertex = {};
                vertex.Position[0] = r * cosf(longitude);
                vertex.Position[1] = r * sinf(longitude);
                vertex.Position[2] = z;
                // 法线 = 归一化的位置向量（单位球体法线）
                // Normal = normalized position vector (unit sphere normal)
                vertex.Normal[0] = vertex.Position[0] / radius;
                vertex.Normal[1] = vertex.Position[1] / radius;
                vertex.Normal[2] = vertex.Position[2] / radius;
                vertex.TexCoord[0] = static_cast<float>(segment) / segments;
                vertex.TexCoord[1] = static_cast<float>(ring) / rings;

                vertices.push_back(vertex);
            }
        }

        // 生成索引 / Generate indices
        for (UINT ring = 0; ring < rings; ++ring)
        {
            for (UINT segment = 0; segment < segments; ++segment)
            {
                UINT topLeft = ring * (segments + 1) + segment;
                UINT topRight = topLeft + 1;
                UINT bottomLeft = (ring + 1) * (segments + 1) + segment;
                UINT bottomRight = bottomLeft + 1;

                // 第一个三角形 / First triangle
                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);

                // 第二个三角形 / Second triangle
                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }

        IndexCount = static_cast<UINT>(indices.size());

        pVertexBuffer = std::make_unique<VertexBufferDX12<DX12Vertex>>(Core, vertices);
        pIndexBuffer = std::make_unique<IndexBufferDX12<UINT>>(Core, indices);
    }

    /**
     * @brief 初始化球体材质 / Initialize sphere material
     *
     * 使用 Color 成员变量作为反照率颜色，自发光材质用于灯光可视化。
     * Uses the Color member variable as albedo color, emissive material for light visualization.
     *
     * @param materialData 材质数据引用 / Material data reference
     */
    void DX12Sphere::InitMaterial(DX12MaterialCB& materialData)
    {
        materialData.Albedo[0] = Color[0];
        materialData.Albedo[1] = Color[1];
        materialData.Albedo[2] = Color[2];
        materialData.Metallic = 0.0f;
        materialData.Roughness = 0.3f;
        materialData.AmbientOcclusion = 1.0f;
        materialData.UseAlbedoTexture = 0;
        materialData.UseRoughnessTexture = 0;
        materialData.UseMetallicTexture = 0;
        materialData.UseNormalTexture = 0;
        materialData.UseAOTexture = 0;
        // 球体作为点光源可视化，不参与光照计算，直接输出纯色
        // Sphere acts as point light visualization, skips lighting and outputs pure color
        materialData.Unlit = 1;
    }

    /**
     * @brief 设置球体颜色 / Set sphere color
     *
     * 设置球体颜色并更新材质常量缓冲区。
     * Sets sphere color and updates material constant buffer.
     *
     * @param r 红色分量 / Red component
     * @param g 绿色分量 / Green component
     * @param b 蓝色分量 / Blue component
     * @param a Alpha 分量 / Alpha component
     */
    void DX12Sphere::SetColor(float r, float g, float b, float a)
    {
        Color[0] = r; Color[1] = g; Color[2] = b; Color[3] = a;

        if (pMaterialBuffer)
        {
            DX12MaterialCB materialData = {};
            InitMaterial(materialData);
            pMaterialBuffer->Update(materialData);
        }
    }

    // =============================================================================
    // DX12WireframeCone
    // DX12 锥体线框（用于聚光灯可视化）
    // =============================================================================

    /**
     * @brief 创建锥体线框几何体 / Create cone wireframe geometry
     *
     * 生成线框锥体，包含侧面边线和底面圆周线。
     * 顶点总数为 1（顶点）+ Segments（底面边缘）。
     * 线数为 2 * Segments（Segments 条侧面边 + Segments 条底面边）。
     * 索引数为 4 * Segments（每条线 2 个索引）。
     *
     * Generates a wireframe cone, including side edges and base circumference lines.
     * Total vertices: 1 (apex) + Segments (base rim).
     * Total lines: 2 * Segments (Segments side edges + Segments base edges).
     * Total indices: 4 * Segments (2 indices per line).
     */
    void DX12WireframeCone::CreateGeometry()
    {
        std::vector<DX12Vertex> vertices;
        std::vector<UINT> indices;

        // 顶点 0：锥体顶点（位于原点，便于对齐光源位置）
        // Vertex 0: Cone apex (at origin, easy to align with light position)
        DX12Vertex apex = {};
        apex.Position[0] = 0.0f;
        apex.Position[1] = 0.0f;
        apex.Position[2] = 0.0f;
        vertices.push_back(apex);

        // 顶点 1..Segments：底面边缘顶点（位于 X = ConeHeight 处，锥体沿 +X 方向延伸）
        // Vertices 1..Segments: Base rim vertices (at X = ConeHeight, cone extends along +X)
        for (UINT i = 0; i < Segments; ++i)
        {
            float angle = static_cast<float>(i) * XM_2PI / Segments;
            DX12Vertex rimVertex = {};
            rimVertex.Position[0] = ConeHeight;
            rimVertex.Position[1] = ConeRadius * cosf(angle);
            rimVertex.Position[2] = ConeRadius * sinf(angle);
            vertices.push_back(rimVertex);
        }

        // 侧面边线：从顶点到底面每个边缘顶点
        // Side edges: from apex to each base rim vertex
        for (UINT i = 0; i < Segments; ++i)
        {
            indices.push_back(0);              // 顶点 / Apex
            indices.push_back(1 + i);          // 底面边缘顶点 / Base rim vertex
        }

        // 底面圆周线：相邻底面边缘顶点之间的连线
        // Base circumference: lines between adjacent base rim vertices
        for (UINT i = 0; i < Segments; ++i)
        {
            UINT rim0 = 1 + i;
            UINT rim1 = 1 + (i + 1) % Segments;
            indices.push_back(rim0);
            indices.push_back(rim1);
        }

        IndexCount = static_cast<UINT>(indices.size());

        pVertexBuffer = std::make_unique<VertexBufferDX12<DX12Vertex>>(Core, vertices);
        pIndexBuffer = std::make_unique<IndexBufferDX12<UINT>>(Core, indices);
    }

    /**
     * @brief 初始化锥体材质 / Initialize cone material
     *
     * 使用 Color 成员变量作为反照率颜色。
     * Uses the Color member variable as albedo color.
     *
     * @param materialData 材质数据引用 / Material data reference
     */
    void DX12WireframeCone::InitMaterial(DX12MaterialCB& materialData)
    {
        materialData.Albedo[0] = Color[0];
        materialData.Albedo[1] = Color[1];
        materialData.Albedo[2] = Color[2];
        materialData.Metallic = 0.0f;
        materialData.Roughness = 0.5f;
        materialData.AmbientOcclusion = 1.0f;
        materialData.UseAlbedoTexture = 0;
        materialData.UseRoughnessTexture = 0;
        materialData.UseMetallicTexture = 0;
        materialData.UseNormalTexture = 0;
        materialData.UseAOTexture = 0;
        materialData.Unlit = 0;
    }

    /**
     * @brief 设置锥体颜色 / Set cone color
     *
     * 设置锥体颜色并更新材质常量缓冲区。
     * Sets cone color and updates material constant buffer.
     *
     * @param r 红色分量 / Red component
     * @param g 绿色分量 / Green component
     * @param b 蓝色分量 / Blue component
     * @param a Alpha 分量 / Alpha component
     */
    void DX12WireframeCone::SetColor(float r, float g, float b, float a)
    {
        Color[0] = r; Color[1] = g; Color[2] = b; Color[3] = a;

        if (pMaterialBuffer)
        {
            DX12MaterialCB materialData = {};
            InitMaterial(materialData);
            pMaterialBuffer->Update(materialData);
        }
    }

    /**
     * @brief 更新锥角 / Update cone angle
     *
     * 根据新的锥体参数重新生成几何体。
     * 锥体高度和底面半径共同决定锥角：tan(angle) = radius / height。
     * Regenerates geometry with new cone parameters.
     * Height and radius together determine the cone angle: tan(angle) = radius / height.
     *
     * @param height 新的锥体高度 / New cone height
     * @param radius 新的底面半径 / New base radius
     */
    void DX12WireframeCone::UpdateAngle(float height, float radius)
    {
        ConeHeight = height;
        ConeRadius = radius;

        // 重新生成线框几何体
        // Regenerate wireframe geometry
        std::vector<DX12Vertex> vertices;
        std::vector<UINT> indices;

        // 顶点 0：锥体顶点（位于原点）/ Vertex 0: Apex (at origin)
        DX12Vertex apex = {};
        apex.Position[0] = 0.0f;
        apex.Position[1] = 0.0f;
        apex.Position[2] = 0.0f;
        vertices.push_back(apex);

        // 顶点 1..Segments：底面边缘顶点（沿 -X 方向）/ Vertices 1..Segments: Base rim (along -X)
        for (UINT i = 0; i < Segments; ++i)
        {
            float angle = static_cast<float>(i) * XM_2PI / Segments;
            DX12Vertex rimVertex = {};
            rimVertex.Position[0] = -ConeHeight;
            rimVertex.Position[1] = ConeRadius * cosf(angle);
            rimVertex.Position[2] = ConeRadius * sinf(angle);
            vertices.push_back(rimVertex);
        }

        // 侧面边线：从顶点到底面每个边缘顶点
        // Side edges: from apex to each base rim vertex
        for (UINT i = 0; i < Segments; ++i)
        {
            indices.push_back(0);
            indices.push_back(1 + i);
        }

        // 底面圆周线：相邻底面边缘顶点之间的连线
        // Base circumference: lines between adjacent base rim vertices
        for (UINT i = 0; i < Segments; ++i)
        {
            UINT rim0 = 1 + i;
            UINT rim1 = 1 + (i + 1) % Segments;
            indices.push_back(rim0);
            indices.push_back(rim1);
        }

        // 使用基类的 RecreateGeometry 更新缓冲区
        // Use base class RecreateGeometry to update buffers
        RecreateGeometry(vertices, indices, static_cast<UINT>(indices.size()));
    }

    /**
     * @brief 绘制锥体线框 / Draw wireframe cone
     *
     * 覆盖基类 Draw 方法，使用 D3D_PRIMITIVE_TOPOLOGY_LINELIST 拓扑
     * 以线框形式渲染锥体。
     *
     * Overrides the base class Draw method, using D3D_PRIMITIVE_TOPOLOGY_LINELIST
     * topology to render the cone as a wireframe.
     *
     * @param commandList 图形命令列表指针 / Graphics command list pointer
     */
    void DX12WireframeCone::UpdateTransformBuffer()
    {
        XMMATRIX modelMatrix = XMMatrixIdentity();

        // 使用正确的变换顺序：缩放 * 旋转 * 平移（S * R * T）
        // 与基类保持一致
        // Correct transform order: scale * rotation * translation (S * R * T)
        // Consistent with base class
        modelMatrix *= XMMatrixScaling(Scale, Scale, Scale);
        modelMatrix *= XMMatrixRotationRollPitchYaw(Rotation[0], Rotation[1], Rotation[2]);
        modelMatrix *= XMMatrixTranslation(Position[0], Position[1], Position[2]);

        XMMATRIX viewMatrix = XMLoadFloat4x4(reinterpret_cast<XMFLOAT4X4*>(ViewMatrix));
        XMMATRIX projMatrix = XMLoadFloat4x4(reinterpret_cast<XMFLOAT4X4*>(ProjectionMatrix));

        XMMATRIX mvpMatrix = modelMatrix * viewMatrix * projMatrix;

        DX12Transform transform = {};

        XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(transform.ModelMatrix), XMMatrixTranspose(modelMatrix));
        XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(transform.ModelViewProjMatrix), XMMatrixTranspose(mvpMatrix));

        if (pTransformBuffer)
        {
            pTransformBuffer->Update(transform);
        }
    }

    void DX12WireframeCone::Draw(ID3D12GraphicsCommandList* commandList)
    {
        // 绑定根签名 / Bind root signature
        if (Core.GetRootSignature() && Core.GetRootSignature()->GetRootSignature())
        {
            commandList->SetGraphicsRootSignature(Core.GetRootSignature()->GetRootSignature());
        }

        // 绑定线管线状态对象（使用 LINELIST 拓扑，匹配 PSO 的 LINE 拓扑类型）
        // Bind line pipeline state object (uses LINELIST topology, matching PSO's LINE topology type)
        // 如果线 PSO 不可用，跳过渲染（避免 D3D12 拓扑类型不匹配错误）
        // If line PSO is not available, skip rendering (avoids D3D12 topology type mismatch error)
        if (Core.GetLinePipelineState() && Core.GetLinePipelineState()->IsInitialized())
        {
            Core.GetLinePipelineState()->Bind(commandList);
        }
        else
        {
            // 没有线 PSO 时，线框锥体无法正确渲染，静默跳过
            // Without line PSO, wireframe cones cannot render correctly, silently skip
            return;
        }

        // 绑定所有根参数（0-2：CBV）/ Bind all root parameters (0-2: CBVs)
        if (pLightCountBuffer)
            pLightCountBuffer->Bind(commandList);
        if (pMaterialBuffer)
            pMaterialBuffer->Bind(commandList);
        if (pTransformBuffer)
            pTransformBuffer->Bind(commandList);

        // 绑定纹理和采样器描述符表 / Bind texture and sampler descriptor tables
        DX12DescriptorHeap* cbvSrvHeap = Core.GetCBVSRVUAVHeap();
        DX12DescriptorHeap* samplerHeap = Core.GetSamplerHeap();
        if (cbvSrvHeap)
        {
            commandList->SetGraphicsRootDescriptorTable(3, cbvSrvHeap->GetGPUHandle(0));
        }

        // 根参数 4：光源缓冲区描述符表（t4-t5）
        // Root param 4: Light buffer descriptor table (t4-t5)
        if (cbvSrvHeap && PointLightBuffer.GetSRVIndex() != UINT_MAX)
        {
            commandList->SetGraphicsRootDescriptorTable(4, cbvSrvHeap->GetGPUHandle(PointLightBuffer.GetSRVIndex()));
        }

        if (samplerHeap)
        {
            commandList->SetGraphicsRootDescriptorTable(5, samplerHeap->GetGPUHandle(0));
        }

        // 绑定顶点缓冲区 / Bind vertex buffer
        if (pVertexBuffer)
            pVertexBuffer->Bind(commandList);

        // 绑定索引缓冲区 / Bind index buffer
        if (pIndexBuffer)
            pIndexBuffer->Bind(commandList);

        // 使用线列表拓扑结构（线框模式）
        // Use line list topology (wireframe mode)
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

        // 执行绘制调用（索引绘制，1个实例）
        // Execute draw call (indexed draw, 1 instance)
        commandList->DrawIndexedInstanced(IndexCount, 1, 0, 0, 0);
    }
}
