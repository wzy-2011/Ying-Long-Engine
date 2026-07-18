/**
 * @file Cone.h
 * @brief 锥体几何体生成类 / Cone geometry generation class
 *
 * 提供基于角度和半径参数的可定制锥体网格生成功能。
 * 锥体默认沿 Z 轴方向，顶点在 Z 正半轴，底面在 Z=0 平面。
 * 支持自定义锥体高度、底面半径、分段数等参数。
 *
 * Provides customizable cone mesh generation based on angle and radius parameters.
 * The cone is oriented along the Z axis by default, with the apex on the positive Z
 * axis and the base on the Z=0 plane. Supports custom height, base radius, segment count, etc.
 */
#pragma once
#include <vector>
#include <DirectXMath.h>
#include "IndexedTriangleList.h"

namespace YingLong
{
	/**
	 * @brief 锥体几何体生成器类 / Cone geometry generator class
	 *
	 * 静态类，通过圆形细分算法生成锥体网格，可指定分段数、高度和底面半径。
	 * Static class that generates cone meshes using circular subdivision algorithm,
	 * with configurable segment count, height, and base radius.
	 */
	class Cone
	{
	public:
		/**
		 * @brief 生成指定参数的锥体 / Generate cone with specified parameters
		 *
		 * 使用圆形细分算法生成锥体网格，仅包含侧面轮廓。
		 * 最小分段数为 3。锥体顶点在 Z = height 处，底面在 Z = 0 处。
		 * 法线应在生成后调用 SetNormalsIndependentFlat() 自动计算。
		 *
		 * Generates a cone mesh using circular subdivision algorithm, side faces only.
		 * Minimum segment count is 3. The apex is at Z = height, the base is at Z = 0.
		 * Normals should be computed automatically via SetNormalsIndependentFlat() after generation.
		 *
		 * @tparam V 顶点类型，必须包含 Position 成员 / Vertex type, must contain Position member
		 * @param segments 底面圆周分段数 / Number of segments around the base circumference
		 * @param height 锥体高度（沿 Z 轴）/ Cone height (along Z axis)
		 * @param radius 底面半径 / Base radius
		 * @return IndexedTriangleList<V> 索引三角形列表 / Indexed triangle list
		 */
		template<typename V>
		static IndexedTriangleList<V> Generate(UINT segments, float height, float radius)
		{
			segments = (segments < 3) ? 3 : segments;

			std::vector<V> vertices;
			std::vector<unsigned short> indices;

			V apex = {};
			apex.Position.x = 0.0f;
			apex.Position.y = 0.0f;
			apex.Position.z = height;
			vertices.push_back(apex);

			for (unsigned int i = 0; i < segments; ++i)
			{
				float angle = static_cast<float>(i) * XM_2PI / segments;
				float x = radius * cosf(angle);
				float y = radius * sinf(angle);

				V rimVertex = {};
				rimVertex.Position.x = x;
				rimVertex.Position.y = y;
				rimVertex.Position.z = 0.0f;

				vertices.push_back(rimVertex);
			}

			for (unsigned int i = 0; i < segments; ++i)
			{
				unsigned int rim0 = 1 + i;
				unsigned int rim1 = 1 + (i + 1) % segments;

				indices.push_back(0);
				indices.push_back(rim0);
				indices.push_back(rim1);
			}

			return { std::move(vertices), std::move(indices) };
		}

		/**
		 * @brief 生成默认参数的锥体（32 段，高度 1，半径 0.5）
		 *        Generate cone with default parameters (32 segments, height 1, radius 0.5)
		 *
		 * @tparam V 顶点类型，必须包含 Position 成员 / Vertex type, must contain Position member
		 * @return IndexedTriangleList<V> 索引三角形列表 / Indexed triangle list
		 */
		template<class V>
		static IndexedTriangleList<V> Make()
		{
			return Generate<V>(32, 1.0f, 0.5f);
		}
	};
}