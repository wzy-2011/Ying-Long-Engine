/**
 * @file IndexedTriangleList.h
 * @brief 索引三角形列表模板类 / Indexed triangle list template class
 *
 * 存储顶点数据和索引数据的三角形网格容器，提供变换和法线计算功能。
 * Triangle mesh container storing vertex data and index data,
 * provides transformation and normal calculation functionality.
 */
#pragma once
#include <vector>
#include <DirectXMath.h>

using namespace DirectX;

/**
 * @brief 索引三角形列表模板类 / Indexed triangle list template class
 *
 * 存储顶点数组和索引数组的网格数据结构，支持顶点变换和平面法线计算。
 * Mesh data structure storing vertex array and index array,
 * supports vertex transformation and flat normal calculation.
 *
 * @tparam T 顶点类型 / Vertex type
 */
template<class T>
class IndexedTriangleList
{
public:
	/**
	 * @brief 默认构造函数 / Default constructor
	 */
	IndexedTriangleList() = default;

	/**
	 * @brief 带参数的构造函数 / Constructor with parameters
	 *
	 * 使用顶点和索引数据初始化三角形列表。
	 * Initializes triangle list with vertex and index data.
	 *
	 * @param verts_in 顶点数据 / Vertex data
	 * @param indices_in 索引数据 / Index data
	 */
	IndexedTriangleList(std::vector<T> verts_in, std::vector<unsigned short> indices_in)
		: vertices(std::move(verts_in)), indices(std::move(indices_in))
	{
		// 确保至少有一个三角形 / Ensure at least one triangle
		assert(vertices.size() > 2);
		// 确保索引数是3的倍数（每个三角形3个索引）
		// Ensure index count is multiple of 3 (3 indices per triangle)
		assert(indices.size() % 3 == 0);
	}

	/**
	 * @brief 变换所有顶点位置 / Transform all vertex positions
	 *
	 * 使用指定的矩阵变换所有顶点的位置坐标。
	 * Transforms all vertex position coordinates using the specified matrix.
	 *
	 * @param matrix 变换矩阵 / Transformation matrix
	 */
	void Transform(FXMMATRIX matrix)
	{
		for (auto& v : vertices)
		{
			// 加载顶点位置并进行矩阵变换 / Load vertex position and apply matrix transformation
			const XMVECTOR pos = XMLoadFloat3(&v.Position);
			XMStoreFloat3(
				&v.Position,
				XMVector3Transform(pos, matrix)
			);
		}
	}

	/**
	 * @brief 设置独立平面法线 / Set independent face normals (flat shading)
	 *
	 * 为每个三角形面计算平面法线并设置到对应顶点。
	 * 要求顶点是面独立的（每个面有自己的顶点）。
	 * Calculates flat normals for each triangle face and sets them to corresponding vertices.
	 * Requires vertices to be face-independent (each face has its own vertices).
	 *
	 * @note 调用此函数前应确保法线已清零 / Ensure normals are cleared before calling this function
	 */
	void SetNormalsIndependentFlat() noexcept
	{
		// 确保索引数量正确 / Ensure correct index count
		assert(indices.size() % 3 == 0 && indices.size() > 0);
		// 遍历每个三角形 / Iterate through each triangle
		for (size_t i = 0; i < indices.size(); i += 3)
		{
			// 获取三角形的三个顶点 / Get three vertices of triangle
			auto& v0 = vertices[indices[i]];
			auto& v1 = vertices[indices[i + 1]];
			auto& v2 = vertices[indices[i + 2]];

			// 加载三个顶点的位置 / Load positions of three vertices
			const auto p0 = DirectX::XMLoadFloat3(&v0.Position);
			const auto p1 = DirectX::XMLoadFloat3(&v1.Position);
			const auto p2 = DirectX::XMLoadFloat3(&v2.Position);

			// 计算叉积得到法线并归一化 / Calculate cross product to get normal and normalize
			const auto n = DirectX::XMVector3Normalize(XMVector3Cross((p1 - p0), (p2 - p0)));

			// 将法线存储回三个顶点 / Store normal back to three vertices
			DirectX::XMStoreFloat3(&v0.Normal, n);
			DirectX::XMStoreFloat3(&v1.Normal, n);
			DirectX::XMStoreFloat3(&v2.Normal, n);
		}
	}

public:
	std::vector<T> vertices;                ///< 顶点数组 / Vertex array
	std::vector<unsigned short> indices;    ///< 索引数组 / Index array
};
