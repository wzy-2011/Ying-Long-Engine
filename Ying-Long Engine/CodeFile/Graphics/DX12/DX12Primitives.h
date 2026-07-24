/**
 * @file DX12Primitives.h
 * @brief DX12 几何图元头文件 / DX12 Geometric Primitives Header
 *
 * 本文件定义了 DX12 几何图元相关的结构体和类，包括：
 * - 顶点结构、变换结构、各种常量缓冲区结构
 * - DX12Primitive 基类（提供通用的顶点/索引/常量缓冲区管理和绘制逻辑）
 * - DX12Triangle 和 DX12Box 等具体图元类
 *
 * This file defines structures and classes related to DX12 geometric primitives, including:
 * - Vertex structure, transform structure, various constant buffer structures
 * - DX12Primitive base class (provides common vertex/index/constant buffer management and draw logic)
 * - Specific primitive classes like DX12Triangle and DX12Box
 */

#pragma once

#include "DX12Core.h"
#include "DX12Drawable.h"
#include "VertexBufferDX12.h"
#include "IndexBufferDX12.h"
#include "ConstantBufferDX12.h"
#include "DX12DescriptorHeap.h"
#include "StructuredBufferDX12.h"
#include <DirectXMath.h>

namespace YingLong
{
    /**
     * @brief DX12 简单顶点结构 / DX12 Simple Vertex Structure
     *
     * 包含位置、法线和纹理坐标的顶点结构，用于简单几何图元。
     * Vertex structure containing position, normal, and texture coordinate,
     * used for simple geometric primitives.
     */
    struct DX12Vertex
    {
        float Position[3];     ///< 顶点位置 / Vertex position
        float Normal[3];       ///< 顶点法线 / Vertex normal
        float TexCoord[2];     ///< 纹理坐标 / Texture coordinate
    };

    /**
     * @brief DX12 变换矩阵结构 / DX12 Transform Matrix Structure
     *
     * 与 PBR 顶点着色器 cbuffer 布局匹配的变换矩阵结构，
     * 包含模型矩阵和 MVP 矩阵。
     *
     * Transform matrix structure matching the PBR vertex shader cbuffer layout,
     * containing the model matrix and MVP matrix.
     */
    struct DX12Transform
    {
        float ModelMatrix[16];         ///< 模型矩阵 / Model matrix
        float ModelViewProjMatrix[16]; ///< 模型视图投影矩阵 / Model-View-Projection matrix
    };

    struct DX12PointLightData
    {
        float Position[3];
        float pad0;
        float Color[3];
        float Intensity;
    };

    struct DX12SpotLightData
    {
        float Position[3];
        float Intensity;
        float Color[3];
        float InnerConeAngle;
        float Direction[3];
        float OuterConeAngle;
        float Rotation[3];   // 未使用，仅用于匹配 HLSL struct 大小
        float pad;           // 未使用，仅用于匹配 HLSL struct 大小
    };

    struct DX12LightCountCB
    {
        int PointLightCount;
        int SpotLightCount;
        float pad[2];
        float CameraPosition[3];
        float pad0;
    };

    /**
     * @brief DX12 材质常量缓冲区结构 / DX12 Material Constant Buffer Structure
     *
     * 与 PBR 像素着色器 b2 寄存器匹配的材质常量缓冲区结构。
     * 包含 PBR 材质参数和纹理使用标志。
     *
     * Material constant buffer structure matching the PBR pixel shader b2 register.
     * Contains PBR material parameters and texture usage flags.
     */
    struct DX12MaterialCB
    {
        float Albedo[3];            ///< 反照率颜色 / Albedo color
        float Metallic;              ///< 金属度 / Metallic
        float Roughness;             ///< 粗糙度 / Roughness
        float AmbientOcclusion;      ///< 环境光遮蔽 / Ambient occlusion
        int UseAlbedoTexture;        ///< 是否使用反照率纹理 / Whether to use albedo texture
        int UseRoughnessTexture;     ///< 是否使用粗糙度纹理 / Whether to use roughness texture
        int UseMetallicTexture;      ///< 是否使用金属度纹理 / Whether to use metallic texture
        int UseNormalTexture;        ///< 是否使用法线纹理 / Whether to use normal texture
        int UseAOTexture;            ///< 是否使用 AO 纹理 / Whether to use AO texture
        float pad[3];                ///< 填充（16字节对齐）/ Padding (16-byte alignment)
    };

    /**
     * @brief DX12 几何图元基类 / DX12 Geometric Primitive Base Class
     *
     * DX12Primitive 是所有 DX12 简单几何图元（三角形、立方体等）的基类。
     * 它持有顶点缓冲区、索引缓冲区、常量缓冲区，以及通用的绑定/绘制/更新逻辑。
     * 派生类只需实现 CreateGeometry()（顶点/索引数据）和 InitMaterial()
     * （每个形状的材质默认值）。
     *
     * 初始化采用两阶段模式：基类构造函数只设置单位矩阵；
     * 派生类构造函数必须在函数体中调用 Initialize() 来调用
     * CreateGeometry() + CreatePlaceholderCBVs()。这是必需的，
     * 因为在基类构造函数运行期间，虚函数派发不会到达派生类。
     *
     * DX12Primitive is the base class for all DX12 simple geometric primitives
     * (triangle, box, etc.). It holds vertex buffer, index buffer, constant buffers,
     * and common bind/draw/update logic. Derived classes only need to implement
     * CreateGeometry() (vertex/index data) and InitMaterial() (per-shape material defaults).
     *
     * Initialization is two-phase: the base constructor only sets identity matrices;
     * derived constructors must call Initialize() in their body to invoke
     * CreateGeometry() + CreatePlaceholderCBVs(). This is required because
     * virtual dispatch does not reach the derived class while the base constructor
     * is still running.
     */
    class DX12Primitive
    {
    public:
        /**
         * @brief 构造函数 / Constructor
         * @param core DX12 核心对象引用 / DX12 core object reference
         */
        explicit DX12Primitive(DX12Core& core);

        /**
         * @brief 析构函数 / Destructor
         */
        virtual ~DX12Primitive() = default;

        /**
         * @brief 更新图元状态 / Update primitive state
         *
         * 根据增量时间更新图元的旋转等动画状态。
         * Updates the primitive's animation state such as rotation based on delta time.
         *
         * @param deltaTime 增量时间（秒）/ Delta time in seconds
         */
        void Update(float deltaTime);

        /**
         * @brief 绘制图元 / Draw primitive
         *
         * 将图元绑定到命令列表并执行绘制调用。
         * Binds the primitive to the command list and executes the draw call.
         *
         * @param commandList 图形命令列表指针 / Graphics command list pointer
         */
        void Draw(ID3D12GraphicsCommandList* commandList);

        /**
         * @brief 设置位置 / Set position
         * @param x X 坐标 / X coordinate
         * @param y Y 坐标 / Y coordinate
         * @param z Z 坐标 / Z coordinate
         */
        void SetPosition(float x, float y, float z);

        /**
         * @brief 设置旋转 / Set rotation
         * @param pitch 俯仰角（弧度）/ Pitch angle in radians
         * @param yaw 偏航角（弧度）/ Yaw angle in radians
         * @param roll 滚转角（弧度）/ Roll angle in radians
         */
        void SetRotation(float pitch, float yaw, float roll);

        /**
         * @brief 设置缩放 / Set scale
         * @param scale 统一缩放值 / Uniform scale value
         */
        void SetScale(float scale);

        /**
         * @brief 设置视图矩阵 / Set view matrix
         * @param viewMatrix 视图矩阵数组（16个元素）/ View matrix array (16 elements)
         */
        void SetViewMatrix(const float* viewMatrix);

        /**
         * @brief 设置投影矩阵 / Set projection matrix
         * @param projMatrix 投影矩阵数组（16个元素）/ Projection matrix array (16 elements)
         */
        void SetProjectionMatrix(const float* projMatrix);

        /**
         * @brief 获取旋转速度数组 / Get rotation speed array
         * @return 旋转速度数组指针（X、Y、Z）/ Rotation speed array pointer (X, Y, Z)
         */
        float* GetRotationSpeed() { return RotationSpeed; }

        /**
         * @brief 设置旋转速度 / Set rotation speed
         * @param pitchSpeed 俯仰角速度（弧度/秒）/ Pitch speed (radians/second)
         * @param yawSpeed 偏航角速度（弧度/秒）/ Yaw speed (radians/second)
         * @param rollSpeed 滚转角速度（弧度/秒）/ Roll speed (radians/second)
         */
        void SetRotationSpeed(float pitchSpeed, float yawSpeed, float rollSpeed)
        {
            RotationSpeed[0] = pitchSpeed;
            RotationSpeed[1] = yawSpeed;
            RotationSpeed[2] = rollSpeed;
        }

        /**
         * @brief 设置光源计数和相机位置数据 / Set light count and camera position data
         *
         * 替换光源计数常量缓冲区内容。
         * 每帧调用一次（在 Draw 之前）以提供 PBR 像素着色器的 b0 cbuffer。
         *
         * Replaces the light count constant buffer contents.
         * Call once per frame (before Draw) to feed PBRPixelShader's b0 cbuffer.
         *
         * @param data 光源计数常量缓冲区数据 / Light count constant buffer data
         */
        void SetLightCountData(const DX12LightCountCB& data);

        /**
         * @brief 初始化静态光源缓冲区 / Initialize static light buffers
         * @param core DX12 核心对象引用 / DX12 core object reference
         * @param initialCapacity 初始容量 / Initial capacity
         */
        static void InitializeLightBuffers(DX12Core& core, UINT initialCapacity);

        /**
         * @brief 更新点光源结构化缓冲区 / Update point light structured buffer
         * @param data 点光源数据向量 / Point light data vector
         */
        static void UpdatePointLightBuffer(const std::vector<DX12PointLightData>& data);

        /**
         * @brief 更新聚光源结构化缓冲区 / Update spot light structured buffer
         * @param data 聚光源数据向量 / Spot light data vector
         */
        static void UpdateSpotLightBuffer(const std::vector<DX12SpotLightData>& data);

        /**
         * @brief 清理静态光源缓冲区资源 / Clean up static light buffer resources
         *
         * 在应用程序关闭时调用，释放静态结构化缓冲区占用的 GPU 资源和描述符。
         * Called on application shutdown to release GPU resources and descriptors
         * occupied by static structured buffers.
         */
        static void CleanupLightBuffers();

        /**
         * @brief 更新光源缓冲区（每帧调用一次） / Update light buffers (called once per frame)
         *
         * 在渲染循环开始时调用，执行光源数据的 GPU 复制操作。
         * Called at the start of the render loop to perform GPU copy operations
         * for light data.
         *
         * @param commandList 命令列表 / Command list
         */
        static void UpdateLightBuffers(ID3D12GraphicsCommandList* commandList);

    protected:
        /**
         * @brief 初始化图元 / Initialize primitive
         *
         * 必须从派生类构造函数的函数体中调用。
         * Must be called from derived constructor's body.
         */
        void Initialize();

        /**
         * @brief 创建几何体（纯虚函数）/ Create geometry (pure virtual)
         *
         * 派生类实现此函数以填充顶点和索引缓冲区。
         * Derived classes implement this to fill vertex and index buffers.
         */
        virtual void CreateGeometry() = 0;

        /**
         * @brief 初始化材质（纯虚函数）/ Initialize material (pure virtual)
         *
         * 派生类实现此函数以填充材质的反照率、金属度、粗糙度、AO 和纹理标志。
         * Derived classes implement this to fill material Albedo/Metallic/Roughness/AO/texture flags.
         *
         * @param materialData 材质常量缓冲区数据引用 / Material constant buffer data reference
         */
        virtual void InitMaterial(DX12MaterialCB& materialData) = 0;

        /**
         * @brief 创建变换缓冲区 / Create transform buffer
         */
        void CreateTransformBuffer();

        /**
     * @brief 创建占位符常量缓冲区 / Create placeholder constant buffers
     *
     * 创建点光源、聚光源和材质的占位符常量缓冲区。
     * Creates placeholder constant buffers for point light, spot light, and material.
     */
    void CreatePlaceholderCBVs();

    /**
     * @brief 重新创建顶点和索引缓冲区（用于动态更新几何体）
     *        Recreate vertex and index buffers (for dynamic geometry updates)
     *
     * 允许派生类在创建后更新几何体数据，特别适用于
     * 锥体线框可视化中锥角随光源参数变化而动态更新。
     *
     * Allows derived classes to update geometry data after creation,
     * especially useful for cone wireframe visualization where the cone
     * angle needs to dynamically update with light parameter changes.
     *
     * @param vertices 新顶点数据 / New vertex data
     * @param indices 新索引数据 / New index data
     * @param indexCount 索引数量 / Index count
     */
    void RecreateGeometry(const std::vector<DX12Vertex>& vertices,
        const std::vector<UINT>& indices, UINT indexCount);

        /**
         * @brief 更新变换缓冲区 / Update transform buffer
         *
         * 根据当前位置、旋转、缩放以及视图和投影矩阵重新计算并更新变换缓冲区。
         * Recalculates and updates the transform buffer based on current position,
         * rotation, scale, and view and projection matrices.
         */
        virtual void UpdateTransformBuffer();

        DX12Core& Core;   ///< DX12 核心对象引用 / DX12 core object reference

        std::unique_ptr<VertexBufferDX12<DX12Vertex>> pVertexBuffer;       ///< 顶点缓冲区 / Vertex buffer
        std::unique_ptr<IndexBufferDX12<UINT>> pIndexBuffer;               ///< 索引缓冲区 / Index buffer
        std::unique_ptr<ConstantBufferDX12<DX12Transform>> pTransformBuffer;    ///< 变换常量缓冲区 / Transform constant buffer
        std::unique_ptr<ConstantBufferDX12<DX12LightCountCB>> pLightCountBuffer; ///< 光源计数常量缓冲区 / Light count constant buffer
        std::unique_ptr<ConstantBufferDX12<DX12MaterialCB>> pMaterialBuffer;    ///< 材质常量缓冲区 / Material constant buffer

        static StructuredBufferDX12<DX12PointLightData> PointLightBuffer;   ///< 点光源结构化缓冲区 / Point light structured buffer
        static StructuredBufferDX12<DX12SpotLightData> SpotLightBuffer;     ///< 聚光源结构化缓冲区 / Spot light structured buffer

        UINT IndexCount = 0;      ///< 索引数量 / Index count

        float Position[3] = { 0.0f, 0.0f, 0.0f };       ///< 位置 / Position
        float Rotation[3] = { 0.0f, 0.0f, 0.0f };       ///< 旋转（弧度）/ Rotation in radians
        float RotationSpeed[3] = { 0.0f, 0.0f, 0.0f };  ///< 旋转速度（弧度/秒）/ Rotation speed in radians/second
        float Scale = 1.0f;                              ///< 缩放 / Scale

        float ViewMatrix[16] = {};       ///< 视图矩阵 / View matrix
        float ProjectionMatrix[16] = {}; ///< 投影矩阵 / Projection matrix
    };

    /**
     * @brief DX12 三角形图元类 / DX12 Triangle Primitive Class
     *
     * 用于测试的简单 DX12 三角形可绘制对象。
     * Simple DX12 triangle drawable for testing purposes.
     */
    class DX12Triangle : public DX12Primitive
    {
    public:
        /**
         * @brief 构造函数 / Constructor
         * @param core DX12 核心对象引用 / DX12 core object reference
         */
        explicit DX12Triangle(DX12Core& core) : DX12Primitive(core) { Initialize(); }
    protected:
        /**
         * @brief 创建三角形几何体 / Create triangle geometry
         */
        void CreateGeometry() override;

        /**
         * @brief 初始化三角形材质 / Initialize triangle material
         * @param materialData 材质数据引用 / Material data reference
         */
        void InitMaterial(DX12MaterialCB& materialData) override;
    };

    /**
     * @brief DX12 立方体/盒体图元类 / DX12 Box/Cube Primitive Class
     *
     * 用于测试的简单 DX12 立方体/盒体可绘制对象。
     * Simple DX12 box/cube drawable for testing purposes.
     */
    class DX12Box : public DX12Primitive
    {
    public:
        /**
         * @brief 构造函数 / Constructor
         * @param core DX12 核心对象引用 / DX12 core object reference
         */
        explicit DX12Box(DX12Core& core) : DX12Primitive(core) { Initialize(); }

        /**
     * @brief 设置颜色 / Set color
     * @param r 红色分量 / Red component
     * @param g 绿色分量 / Green component
     * @param b 蓝色分量 / Blue component
     * @param a Alpha 分量 / Alpha component
     */
    void SetColor(float r, float g, float b, float a);
protected:
    /**
     * @brief 创建立方体几何体 / Create box geometry
     */
    void CreateGeometry() override;

    /**
     * @brief 初始化立方体材质 / Initialize box material
     * @param materialData 材质数据引用 / Material data reference
     */
    void InitMaterial(DX12MaterialCB& materialData) override;
private:
    float Color[4] = { 0.5f, 0.5f, 0.5f, 1.0f };  ///< 颜色 / Color
};

    /**
     * @brief DX12 球体图元类（用于点光源可视化）/ DX12 Sphere Primitive Class (for point light visualization)
     *
     * 用于在 DX12 渲染环境下可视化点光源的位置和颜色。
     * 球体颜色通过 SetColor() 设置，与点光源颜色保持同步。
     *
     * Used for visualizing point light position and color in DX12 render mode.
     * Sphere color is set via SetColor(), synced with point light color.
     */
    class DX12Sphere : public DX12Primitive
    {
    public:
        /**
         * @brief 构造函数 / Constructor
         * @param core DX12 核心对象引用 / DX12 core object reference
         */
        explicit DX12Sphere(DX12Core& core) : DX12Primitive(core) { Initialize(); }

        /**
         * @brief 设置颜色 / Set color
         * @param r 红色分量 / Red component
         * @param g 绿色分量 / Green component
         * @param b 蓝色分量 / Blue component
         * @param a Alpha 分量 / Alpha component
         */
        void SetColor(float r, float g, float b, float a);
    protected:
        /**
         * @brief 创建球体几何体 / Create sphere geometry
         */
        void CreateGeometry() override;

        /**
         * @brief 初始化球体材质 / Initialize sphere material
         * @param materialData 材质数据引用 / Material data reference
         */
        void InitMaterial(DX12MaterialCB& materialData) override;
    private:
        float Color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };  ///< 颜色 / Color
    };

    /**
     * @brief DX12 锥体线框图元类（用于聚光灯可视化）
     *        DX12 Wireframe Cone Primitive Class (for spot light visualization)
     *
     * 用于在 DX12 渲染环境下可视化聚光灯的位置、方向和锥角。
     * 锥体以线框形式渲染，颜色与聚光灯颜色保持同步。
     * 锥体的开口角度可通过 UpdateAngle() 动态更新以匹配聚光灯外锥角。
     *
     * Used for visualizing spot light position, direction, and cone angle in
     * DX12 render mode. The cone is rendered as a wireframe, color synced with
     * the spot light color. The cone opening angle can be dynamically updated
     * via UpdateAngle() to match the spot light's outer cone angle.
     */
    class DX12WireframeCone : public DX12Primitive
    {
    public:
        /**
         * @brief 构造函数 / Constructor
         * @param core DX12 核心对象引用 / DX12 core object reference
         * @param height 锥体高度（沿 X 轴）/ Cone height (along X axis)
         * @param radius 底面半径 / Base radius
         * @param segments 圆周分段数 / Circumference segment count
         */
        explicit DX12WireframeCone(DX12Core& core, float height = 1.0f, float radius = 0.5f, UINT segments = 32)
            : DX12Primitive(core), ConeHeight(height), ConeRadius(radius), Segments(segments) { Initialize(); }

        /**
         * @brief 设置颜色 / Set color
         * @param r 红色分量 / Red component
         * @param g 绿色分量 / Green component
         * @param b 蓝色分量 / Blue component
         * @param a Alpha 分量 / Alpha component
         */
        void SetColor(float r, float g, float b, float a);

        /**
         * @brief 更新锥角 / Update cone angle
         *
         * 根据新的锥角重新生成锥体几何体。
         * 锥角通过锥体高度和底面半径确定。
         * Regenerates cone geometry with new cone angle.
         * Cone angle is determined by height and base radius.
         *
         * @param height 锥体高度 / Cone height
         * @param radius 底面半径 / Base radius
         */
        void UpdateAngle(float height, float radius);

        /**
         * @brief 获取当前锥体高度 / Get current cone height
         * @return float 锥体高度 / Cone height
         */
        float GetHeight() const { return ConeHeight; }

        /**
         * @brief 获取当前底面半径 / Get current base radius
         * @return float 底面半径 / Base radius
         */
        float GetRadius() const { return ConeRadius; }

        /**
         * @brief 绘制锥体线框 / Draw wireframe cone
         * @param commandList 图形命令列表指针 / Graphics command list pointer
         *
         * 覆盖基类 Draw 方法，使用 D3D_PRIMITIVE_TOPOLOGY_LINELIST 拓扑。
         * Overrides the base class Draw method, using D3D_PRIMITIVE_TOPOLOGY_LINELIST topology.
         */
        void Draw(ID3D12GraphicsCommandList* commandList);

    protected:
        /**
         * @brief 创建锥体线框几何体 / Create cone wireframe geometry
         *
         * 生成线框锥体的顶点和索引数据。
         * 包含侧面边线和底面圆周线。
         * Generates vertex and index data for the wireframe cone.
         * Includes side edges and base circumference lines.
         */
        void CreateGeometry() override;

        /**
         * @brief 初始化材质 / Initialize material
         * @param materialData 材质数据引用 / Material data reference
         */
        void InitMaterial(DX12MaterialCB& materialData) override;

        /**
         * @brief 更新变换缓冲区（重写基类方法）
         *
         * 使用正确的变换顺序：先旋转再平移，确保锥体顶点固定在光源位置。
         * Uses correct transform order: rotate first, then translate, ensuring cone apex stays at light position.
         */
        void UpdateTransformBuffer();

    private:
        float Color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };  ///< 颜色 / Color
        float ConeHeight;       ///< 锥体高度 / Cone height
        float ConeRadius;       ///< 底面半径 / Base radius
        UINT Segments;          ///< 圆周分段数 / Circumference segments
    };
}
