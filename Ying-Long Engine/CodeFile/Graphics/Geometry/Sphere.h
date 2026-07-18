/**
 * @file Sphere.h
 * @brief 球体几何体生成类 / Sphere geometry generation class
 *
 * 提供基于经纬度细分的球体网格生成功能。
 * Provides sphere mesh generation based on latitude and longitude subdivision.
 */
#pragma once
#include <initializer_list>
#include "IndexedTriangleList.h"
#include <DirectXMath.h>

namespace YingLong
{
	/**
	 * @brief 球体几何体生成器类 / Sphere geometry generator class
	 *
	 * 静态类，通过经纬度细分算法生成球体网格，可指定分段数和环数。
	 * Static class that generates sphere meshes using latitude/longitude subdivision
	 * algorithm, with configurable segment and ring counts.
	 */
	class Sphere
	{
	public:
		/**
		 * @brief 生成指定精度的球体 / Generate sphere with specified precision
		 *
		 * 使用经纬度细分算法生成球体网格，最小分段和环数为3。
		 * Generates sphere mesh using latitude/longitude subdivision algorithm,
		 * minimum 3 segments and 3 rings.
		 *
		 * @tparam V 顶点类型，必须包含 Position 成员 / Vertex type, must contain Position member
		 * @param segments 经度方向分段数 / Number of segments in longitude direction
		 * @param rings 纬度方向环数 / Number of rings in latitude direction
		 * @return IndexedTriangleList<V> 索引三角形列表 / Indexed triangle list
		 */
		template<typename V>
		static IndexedTriangleList<V> Generate(UINT segments, UINT rings)
		{
			// 确保最小分段数为3 / Ensure minimum segment count is 3
			segments = (segments < 3) ? 3 : segments;
			rings = (rings < 3) ? 3 : rings;

			// 球体半径为1（单位球）/ Sphere radius is 1 (unit sphere)
			float radius = 1.0f;

			std::vector<V> vertices;
			vertices.clear();
			std::vector<USHORT> indices;
			indices.clear();

			// 生成顶点 / Generate vertices
			// 遍历纬度环（从北极到南极）/ Iterate through latitude rings (from north to south pole)
			for (unsigned int ring = 0; ring <= rings; ++ring)
			{
				// 计算纬度角：从 π/2（北极）到 -π/2（南极）
				// Calculate latitude angle: from π/2 (north pole) to -π/2 (south pole)
				float latitude = XM_PIDIV2 - static_cast<float>(ring) * XM_PI / rings;
				float z = radius * sinf(latitude);  // Z轴坐标（高度）/ Z-axis coordinate (height)
				float r = radius * cosf(latitude);  // 当前纬度圈的半径 / Radius at current latitude

				// 遍历经度分段 / Iterate through longitude segments
				for (unsigned int segment = 0; segment <= segments; ++segment)
				{
					// 计算经度角：从 0 到 2π
					// Calculate longitude angle: from 0 to 2π
					float longitude = static_cast<float>(segment) * XM_2PI / segments;

					V vertex = {};
					vertex.Position.x = r * cosf(longitude);
					vertex.Position.y = r * sinf(longitude);
					vertex.Position.z = z;

					vertices.push_back(vertex);
				}
			}

			// 生成索引 / Generate indices
			// 每个四边形由两个三角形组成 / Each quad consists of two triangles
			for (unsigned int ring = 0; ring < rings; ++ring)
			{
				for (unsigned int segment = 0; segment < segments; ++segment)
				{
					// 四个角点索引 / Four corner indices
					unsigned int topLeft = ring * (segments + 1) + segment;
					unsigned int topRight = topLeft + 1;
					unsigned int bottomLeft = (ring + 1) * (segments + 1) + segment;
					unsigned int bottomRight = bottomLeft + 1;

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

			return { std::move(vertices), std::move(indices) };
		}

		/**
		 * @brief 生成默认精度的球体（32×32）/ Generate sphere with default precision (32×32)
		 *
		 * 使用默认的32段和32环生成标准球体。
		 * Generates standard sphere using default 32 segments and 32 rings.
		 *
		 * @tparam V 顶点类型，必须包含 Position 成员 / Vertex type, must contain Position member
		 * @return IndexedTriangleList<V> 索引三角形列表 / Indexed triangle list
		 */
		template<class V>
		static IndexedTriangleList<V> Make()
		{
			return Generate<V>(32, 32);
		}
	};
}
