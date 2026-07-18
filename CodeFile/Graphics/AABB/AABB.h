/**
 * @file AABB.h
 * @brief 轴对齐包围盒类 / Axis-Aligned Bounding Box class
 *
 * 提供AABB的表示及与射线的相交检测功能。
 * Provides AABB representation and ray intersection detection functionality.
 */
#pragma once
#include <DirectXMath.h>
#include <algorithm>

using namespace DirectX;

namespace YingLong
{
	/**
	 * @brief 轴对齐包围盒类 / Axis-Aligned Bounding Box class
	 *
	 * 表示三维空间中的轴对齐包围盒，存储最大和最小顶点坐标，
	 * 提供与射线相交检测等功能。
	 * Represents an axis-aligned bounding box in 3D space, storing max and min
	 * vertex coordinates, provides ray intersection detection and other features.
	 */
	class AABB
	{
	public:
		/**
		 * @brief 默认构造函数 / Default constructor
		 *
		 * 创建一个无效的空包围盒（Max为负无穷，Min为正无穷）。
		 * Creates an invalid empty bounding box (Max is -infinity, Min is +infinity).
		 */
		AABB();

		/**
		 * @brief 带参数的构造函数 / Constructor with parameters
		 *
		 * 使用最大和最小坐标创建包围盒。
		 * Creates bounding box using max and min coordinates.
		 *
		 * @param max 包围盒最大点坐标 / Maximum point coordinates
		 * @param min 包围盒最小点坐标 / Minimum point coordinates
		 */
		AABB(XMFLOAT3 max, XMFLOAT3 min);

		/**
		 * @brief 拷贝构造函数 / Copy constructor
		 *
		 * @param other 另一个AABB对象 / Another AABB object
		 */
		AABB(const AABB& other);

		/**
		 * @brief 设置包围盒最小点坐标 / Set minimum point coordinates of bounding box
		 *
		 * @param min 最小点坐标 / Minimum point coordinates
		 */
		void SetMin(XMFLOAT3 min) noexcept;

		/**
		 * @brief 设置包围盒最大点坐标 / Set maximum point coordinates of bounding box
		 *
		 * @param max 最大点坐标 / Maximum point coordinates
		 */
		void SetMax(XMFLOAT3 max) noexcept;

		/**
		 * @brief 检测射线与包围盒是否相交
		 *        Check if ray intersects with bounding box
		 *
		 * 使用slab方法计算射线与AABB的相交。
		 * Uses slab method to calculate ray-AABB intersection.
		 *
		 * @param rayOrigin 射线起点 / Ray origin
		 * @param rayDirection 射线方向（应该是归一化的）/ Ray direction (should be normalized)
		 * @param distance 输出相交距离（可选，为nullptr时不输出）
		 *        / Output intersection distance (optional, not output when nullptr)
		 * @return bool 是否相交 / Whether intersection occurs
		 */
		bool Intersect(XMFLOAT3 rayOrigin, XMFLOAT3 rayDirection, float* distance = nullptr);

		XMFLOAT3 Max;    ///< 包围盒最大点坐标 / Maximum point of bounding box
		XMFLOAT3 Min;    ///< 包围盒最小点坐标 / Minimum point of bounding box
	};
}
