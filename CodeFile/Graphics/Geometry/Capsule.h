/**
 * @file Capsule.h
 * @brief 胶囊体几何体生成类 / Capsule geometry generation class
 *
 * 提供由圆柱体和两个半球组成的胶囊体网格生成功能。
 * Provides capsule mesh generation consisting of a cylinder and two hemispheres.
 */
#pragma once
#include "IndexedTriangleList.h"
#include <DirectXMath.h>

#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

namespace YingLong
{
	/**
	 * @brief 胶囊体几何体生成器类 / Capsule geometry generator class
	 *
	 * 静态类，生成由圆柱体和上下两个半球组成的胶囊体网格，支持蒙皮纹理坐标。
	 * Static class that generates capsule meshes consisting of a cylinder and
	 * top/bottom hemispheres, supports textured coordinates for skinning.
	 */
	class Capsule
	{
	public:
		/**
		 * @brief 生成胶囊体几何体 / Generate capsule geometry
		 *
		 * 生成由中间圆柱体和上下两个半球组成的胶囊体。
		 * Generates a capsule consisting of a middle cylinder and top/bottom hemispheres.
		 *
		 * @tparam V 顶点类型，必须包含 Position 成员 / Vertex type, must contain Position member
		 * @param radius 胶囊体半径 / Capsule radius
		 * @param halfHeight 胶囊体半高（从中心到顶端）/ Half height of capsule (from center to top)
		 * @param segments 圆周方向分段数 / Number of segments around circumference
		 * @param stacks 半球堆叠层数 / Number of stacks for hemispheres
		 * @return IndexedTriangleList<V> 索引三角形列表 / Indexed triangle list
		 */
		template<class V>
		static IndexedTriangleList<V> Make(float radius = 0.15f,
			float halfHeight = 0.15f, unsigned int segments = 32, unsigned int stacks = 16)
		{
			// 确保参数最小值 / Ensure minimum parameter values
			segments = MAX(4u, segments);  // 分段最小值 / Minimum segments
			stacks = MAX(2u, stacks);      // 堆叠最小值 / Minimum stacks

			std::vector<V> vertices;
			std::vector<unsigned short> indices;

			// 计算圆柱体高度（总高度减去两个半球的半径）
			// Calculate cylinder height (total height minus two hemisphere radii)
			const float cylinderHeight = 2.0f * (halfHeight - radius);
			const float topCylinderY = halfHeight - radius;    // 圆柱体顶部Y坐标 / Cylinder top Y coordinate
			const float bottomCylinderY = -topCylinderY;       // 圆柱体底部Y坐标 / Cylinder bottom Y coordinate

			// 1. 生成圆柱体面段顶点
			// 思路：圆柱侧面使用和半球相同的经度分段，保证接缝平滑
			// 1. Generate cylinder face vertices
			// Idea: Cylinder side uses same longitude segments as hemispheres for smooth seams
			for (unsigned int seg = 0; seg < segments; ++seg)
			{
				const float angle = (float)seg / segments * DirectX::XM_2PI;
				const float x = radius * cosf(angle);
				const float z = radius * sinf(angle);

				// 圆柱体底部顶点 / Cylinder bottom vertex
				V vBottom{};
				vBottom.Position = { x, bottomCylinderY, z };
				vertices.push_back(vBottom);

				// 圆柱体顶部顶点 / Cylinder top vertex
				V vTop{};
				vTop.Position = { x, topCylinderY, z };
				vertices.push_back(vTop);
			}

			// 2. 生成上下两个半球顶点
			// 思路：半球由多层堆叠(stacks)构成，纬度从0到PI/2
			// 2. Generate top and bottom hemisphere vertices
			// Idea: Hemisphere consists of multiple stacks, latitude from 0 to PI/2
			auto AddHemisphere = [&](float centerY, bool isTop)
				{
					// 每层堆叠有segments个顶点，共stacks+1层
					// Each stack has segments vertices, stacks+1 layers total
					for (unsigned int stack = 0; stack <= stacks; ++stack)
					{
						// 纬度角度：上半球从0到PI/2，下半球从0到-PI/2
						// Latitude angle: top hemisphere 0 to PI/2, bottom hemisphere 0 to -PI/2
						const float latAngle = isTop ?
							(float)stack / stacks * DirectX::XM_PIDIV2 :
							-(float)stack / stacks * DirectX::XM_PIDIV2;

						const float y = radius * sinf(latAngle) + centerY;
						const float r = radius * cosf(latAngle);  // 当前纬度圈的半径 / Radius at current latitude

						// 生成当前堆叠层的所有经度段顶点
						// Generate all longitude segment vertices for current stack layer
						for (unsigned int seg = 0; seg < segments; ++seg)
						{
							const float lonAngle = (float)seg / segments * DirectX::XM_2PI;
							V v{};
							v.Position = {
								r * cosf(lonAngle),
								y,
								r * sinf(lonAngle)
							};
							vertices.push_back(v);
						}
					}
				};

			// 添加上半球（连接圆柱体顶部）和下半球（连接圆柱体底部）
			// Add top hemisphere (connects to cylinder top) and bottom hemisphere (connects to cylinder bottom)
			AddHemisphere(topCylinderY, true);
			AddHemisphere(bottomCylinderY, false);

			// 计算各部分顶点索引辅助变量
			// Calculate vertex index helper variables for each part
			const unsigned int cylinderVertexCount = 2 * segments;                  // 圆柱体顶点数 / Cylinder vertex count
			const unsigned int hemisphereVertexCount = (stacks + 1) * segments;      // 单个半球顶点数 / Single hemisphere vertex count
			const unsigned int lowerHemisphereStart = cylinderVertexCount;           // 下半球起始索引 / Lower hemisphere start index
			const unsigned int upperHemisphereStart = lowerHemisphereStart + hemisphereVertexCount;  // 上半球起始索引 / Upper hemisphere start index

			// 3. 生成圆柱侧面索引
			// 思路：使用两个三角形分割一个四边形的方式
			// 3. Generate cylinder side indices
			// Idea: Split each quad into two triangles
			for (unsigned int seg = 0; seg < segments; ++seg)
			{
				const unsigned int nextSeg = (seg + 1) % segments;

				// 圆柱体底部两个顶点索引 / Cylinder bottom two vertex indices
				const unsigned int bottomCurrent = 2 * seg;
				const unsigned int bottomNext = 2 * nextSeg;

				// 圆柱体顶部两个顶点索引 / Cylinder top two vertex indices
				const unsigned int topCurrent = bottomCurrent + 1;
				const unsigned int topNext = bottomNext + 1;

				// 第一个三角形：底部当前 -> 顶部当前 -> 顶部下一个
				// First triangle: bottom current -> top current -> top next
				indices.push_back(bottomCurrent);
				indices.push_back(topCurrent);
				indices.push_back(topNext);

				// 第二个三角形：底部当前 -> 顶部下一个 -> 底部下一个
				// Second triangle: bottom current -> top next -> bottom next
				indices.push_back(bottomCurrent);
				indices.push_back(topNext);
				indices.push_back(bottomNext);
			}

			// 4. 下半球与圆柱体底部的接缝连接
			// 4. Seam connection between lower hemisphere and cylinder bottom
			for (unsigned int seg = 0; seg < segments; ++seg)
			{
				const unsigned int nextSeg = (seg + 1) % segments;
				const unsigned int cylinderBottomCurrent = 2 * seg;          // 圆柱底部当前顶点 / Cylinder bottom current vertex
				const unsigned int cylinderBottomNext = 2 * nextSeg;         // 圆柱底部下一个顶点 / Cylinder bottom next vertex

				// 下半球第一层（与圆柱连接的第一层）的顶点
				// Lower hemisphere first layer (first layer connecting to cylinder) vertices
				const unsigned int hemiCurrent = lowerHemisphereStart + seg;
				const unsigned int hemiNext = lowerHemisphereStart + nextSeg;

				// 接缝连接三角形 / Seam connection triangles
				indices.push_back(cylinderBottomCurrent);
				indices.push_back(hemiCurrent);
				indices.push_back(hemiNext);

				indices.push_back(cylinderBottomCurrent);
				indices.push_back(hemiNext);
				indices.push_back(cylinderBottomNext);
			}

			// 5. 下半球内部索引
			// 思路：堆叠层之间通过四边形连接
			// 5. Lower hemisphere internal indices
			// Idea: Connect stack layers via quads
			for (unsigned int stack = 0; stack < stacks; ++stack)
			{
				for (unsigned int seg = 0; seg < segments; ++seg)
				{
					const unsigned int nextSeg = (seg + 1) % segments;
					const unsigned int currentStackStart = lowerHemisphereStart + stack * segments;
					const unsigned int nextStackStart = lowerHemisphereStart + (stack + 1) * segments;

					const unsigned int a = currentStackStart + seg;
					const unsigned int b = currentStackStart + nextSeg;
					const unsigned int c = nextStackStart + seg;
					const unsigned int d = nextStackStart + nextSeg;

					// 下半球三角形（注意缠绕方向）
					// Lower hemisphere triangles (note winding direction)
					indices.push_back(a);
					indices.push_back(c);
					indices.push_back(d);

					indices.push_back(a);
					indices.push_back(d);
					indices.push_back(b);
				}
			}

			// 6. 上半球与圆柱体顶部的接缝连接
			// 6. Seam connection between upper hemisphere and cylinder top
			for (unsigned int seg = 0; seg < segments; ++seg)
			{
				const unsigned int nextSeg = (seg + 1) % segments;
				const unsigned int cylinderTopCurrent = 2 * seg + 1;  // 圆柱顶部当前顶点 / Cylinder top current vertex
				const unsigned int cylinderTopNext = 2 * nextSeg + 1; // 圆柱顶部下一个顶点 / Cylinder top next vertex

				// 上半球第一层（与圆柱连接的第一层）的顶点
				// Upper hemisphere first layer (first layer connecting to cylinder) vertices
				const unsigned int hemiCurrent = upperHemisphereStart + seg;
				const unsigned int hemiNext = upperHemisphereStart + nextSeg;

				// 接缝连接三角形 / Seam connection triangles
				indices.push_back(cylinderTopCurrent);
				indices.push_back(hemiNext);
				indices.push_back(hemiCurrent);

				indices.push_back(cylinderTopCurrent);
				indices.push_back(cylinderTopNext);
				indices.push_back(hemiNext);
			}

			// 7. 上半球内部索引
			// 7. Upper hemisphere internal indices
			for (unsigned int stack = 0; stack < stacks; ++stack)
			{
				for (unsigned int seg = 0; seg < segments; ++seg)
				{
					const unsigned int nextSeg = (seg + 1) % segments;
					const unsigned int currentStackStart = upperHemisphereStart + stack * segments;
					const unsigned int nextStackStart = upperHemisphereStart + (stack + 1) * segments;

					const unsigned int a = currentStackStart + seg;
					const unsigned int b = currentStackStart + nextSeg;
					const unsigned int c = nextStackStart + seg;
					const unsigned int d = nextStackStart + nextSeg;

					// 上半球三角形（注意缠绕方向与下半球相反）
					// Upper hemisphere triangles (note winding direction is opposite to lower hemisphere)
					indices.push_back(a);
					indices.push_back(d);
					indices.push_back(c);

					indices.push_back(a);
					indices.push_back(b);
					indices.push_back(d);
				}
			}

			return { std::move(vertices), std::move(indices) };
		}

		/**
		 * @brief 生成带纹理坐标的胶囊体（蒙皮版本）
		 *        Generate capsule with texture coordinates (skinned version)
		 *
		 * 生成带有完整纹理UV坐标的胶囊体，支持材质贴图。
		 * Generates a capsule with complete texture UV coordinates, supports material mapping.
		 *
		 * @tparam V 顶点类型，必须包含 Position 和 TextureCoord 成员
		 *         / Vertex type, must contain Position and TextureCoord members
		 * @param radius 胶囊体半径 / Capsule radius
		 * @param halfHeight 胶囊体半高 / Half height of capsule
		 * @param segments 圆周方向分段数 / Number of segments around circumference
		 * @param stacks 半球堆叠层数 / Number of stacks for hemispheres
		 * @return IndexedTriangleList<V> 带纹理坐标的索引三角形列表
		 *         / Indexed triangle list with texture coordinates
		 */
		template<class V>
		static IndexedTriangleList<V> MakeSkinned(float radius = 0.15f,
			float halfHeight = 0.15f, unsigned int segments = 32, unsigned int stacks = 16)
		{
			segments = MAX(4u, segments);
			stacks = MAX(2u, stacks);

			std::vector<V> vertices;
			std::vector<unsigned short> indices;

			const float cylinderHeight = 2.0f * (halfHeight - radius);
			const float topCylinderY = halfHeight - radius;
			const float bottomCylinderY = -topCylinderY;

			// 1. 圆柱体面段顶点（带纹理坐标）
			// 1. Cylinder face vertices (with texture coordinates)
			for (unsigned int seg = 0; seg < segments; ++seg)
			{
				const float angle = (float)seg / segments * DirectX::XM_2PI;
				const float x = radius * cosf(angle);
				const float z = radius * sinf(angle);
				const float u = (float)seg / segments;  // 纹理U坐标(0~1) / Texture U coordinate (0~1)

				// 圆柱体底部顶点（V坐标0.25）/ Cylinder bottom vertex (V coordinate 0.25)
				V vBottom{};
				vBottom.Position = { x, bottomCylinderY, z };
				vBottom.TextureCoord = { u, 0.25f };
				vertices.push_back(vBottom);

				// 圆柱体顶部顶点（V坐标0.75）/ Cylinder top vertex (V coordinate 0.75)
				V vTop{};
				vTop.Position = { x, topCylinderY, z };
				vTop.TextureCoord = { u, 0.75f };
				vertices.push_back(vTop);
			}

			// 2. 生成带纹理坐标的半球
			// 2. Generate hemisphere with texture coordinates
			auto AddHemisphere = [&](float centerY, bool isTop)
				{
					for (unsigned int stack = 0; stack <= stacks; ++stack)
					{
						const float latAngle = isTop ?
							(float)stack / stacks * DirectX::XM_PIDIV2 :
							-(float)stack / stacks * DirectX::XM_PIDIV2;

						const float y = radius * sinf(latAngle) + centerY;
						const float r = radius * cosf(latAngle);
						const float vRatio = (float)stack / stacks;  // 堆叠比例0~1 / Stack ratio 0~1

						for (unsigned int seg = 0; seg < segments; ++seg)
						{
							const float lonAngle = (float)seg / segments * DirectX::XM_2PI;
							const float u = (float)seg / segments;

							// 纹理V坐标分布：上半球(0.75~1.0)，下半球(0.0~0.25)
							// Texture V coordinate distribution: top hemisphere (0.75~1.0), bottom hemisphere (0.0~0.25)
							const float v = isTop ?
								0.75f + vRatio * 0.25f :
								0.25f - vRatio * 0.25f;

							V vtx{};
							vtx.Position = { r * cosf(lonAngle), y, r * sinf(lonAngle) };
							vtx.TextureCoord = { u, v };
							vertices.push_back(vtx);
						}
					}
				};

			AddHemisphere(topCylinderY, true);
			AddHemisphere(bottomCylinderY, false);

			// 计算索引辅助值 / Calculate index helper values
			const unsigned int cylinderVertexCount = 2 * segments;
			const unsigned int hemisphereVertexCount = (stacks + 1) * segments;
			const unsigned int lowerHemisphereStart = cylinderVertexCount;
			const unsigned int upperHemisphereStart = lowerHemisphereStart + hemisphereVertexCount;

			// 3. 圆柱侧面索引 / 3. Cylinder side indices
			for (unsigned int seg = 0; seg < segments; ++seg)
			{
				const unsigned int nextSeg = (seg + 1) % segments;
				const unsigned int bottomCurrent = 2 * seg;
				const unsigned int bottomNext = 2 * nextSeg;
				const unsigned int topCurrent = bottomCurrent + 1;
				const unsigned int topNext = bottomNext + 1;

				indices.push_back(bottomCurrent);
				indices.push_back(topCurrent);
				indices.push_back(topNext);
				indices.push_back(bottomCurrent);
				indices.push_back(topNext);
				indices.push_back(bottomNext);
			}

			// 4. 下半球与圆柱底部接缝 / 4. Lower hemisphere to cylinder bottom seam
			for (unsigned int seg = 0; seg < segments; ++seg)
			{
				const unsigned int nextSeg = (seg + 1) % segments;
				const unsigned int cylinderBottomCurrent = 2 * seg;
				const unsigned int cylinderBottomNext = 2 * nextSeg;
				const unsigned int hemiCurrent = lowerHemisphereStart + seg;
				const unsigned int hemiNext = lowerHemisphereStart + nextSeg;

				indices.push_back(cylinderBottomCurrent);
				indices.push_back(hemiCurrent);
				indices.push_back(hemiNext);
				indices.push_back(cylinderBottomCurrent);
				indices.push_back(hemiNext);
				indices.push_back(cylinderBottomNext);
			}

			// 5. 下半球内部 / 5. Lower hemisphere internal
			for (unsigned int stack = 0; stack < stacks; ++stack)
			{
				for (unsigned int seg = 0; seg < segments; ++seg)
				{
					const unsigned int nextSeg = (seg + 1) % segments;
					const unsigned int currentStackStart = lowerHemisphereStart + stack * segments;
					const unsigned int nextStackStart = lowerHemisphereStart + (stack + 1) * segments;

					const unsigned int a = currentStackStart + seg;
					const unsigned int b = currentStackStart + nextSeg;
					const unsigned int c = nextStackStart + seg;
					const unsigned int d = nextStackStart + nextSeg;

					indices.push_back(a);
					indices.push_back(c);
					indices.push_back(d);
					indices.push_back(a);
					indices.push_back(d);
					indices.push_back(b);
				}
			}

			// 6. 上半球与圆柱顶部接缝 / 6. Upper hemisphere to cylinder top seam
			for (unsigned int seg = 0; seg < segments; ++seg)
			{
				const unsigned int nextSeg = (seg + 1) % segments;
				const unsigned int cylinderTopCurrent = 2 * seg + 1;
				const unsigned int cylinderTopNext = 2 * nextSeg + 1;
				const unsigned int hemiCurrent = upperHemisphereStart + seg;
				const unsigned int hemiNext = upperHemisphereStart + nextSeg;

				indices.push_back(cylinderTopCurrent);
				indices.push_back(hemiNext);
				indices.push_back(hemiCurrent);
				indices.push_back(cylinderTopCurrent);
				indices.push_back(cylinderTopNext);
				indices.push_back(hemiNext);
			}

			// 7. 上半球内部 / 7. Upper hemisphere internal
			for (unsigned int stack = 0; stack < stacks; ++stack)
			{
				for (unsigned int seg = 0; seg < segments; ++seg)
				{
					const unsigned int nextSeg = (seg + 1) % segments;
					const unsigned int currentStackStart = upperHemisphereStart + stack * segments;
					const unsigned int nextStackStart = upperHemisphereStart + (stack + 1) * segments;

					const unsigned int a = currentStackStart + seg;
					const unsigned int b = currentStackStart + nextSeg;
					const unsigned int c = nextStackStart + seg;
					const unsigned int d = nextStackStart + nextSeg;

					indices.push_back(a);
					indices.push_back(d);
					indices.push_back(c);
					indices.push_back(a);
					indices.push_back(b);
					indices.push_back(d);
				}
			}

			return { std::move(vertices), std::move(indices) };
		}
	};
}
