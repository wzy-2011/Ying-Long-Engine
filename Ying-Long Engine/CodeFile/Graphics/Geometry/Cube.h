/**
 * @file Cube.h
 * @brief 立方体几何体生成类 / Cube geometry generation class
 *
 * 提供多种方式生成立方体网格数据，包括基础版本、带纹理坐标版本和独立面版本。
 * Provides multiple ways to generate cube mesh data, including basic version,
 * textured version, and independent faces version.
 */
#pragma once
#include <initializer_list>
#include <DirectXMath.h>
#include "IndexedTriangleList.h"

namespace YingLong
{
	/**
	 * @brief 立方体几何体生成器类 / Cube geometry generator class
	 *
	 * 静态类，提供多种生成立方体网格的方法，顶点格式可通过模板参数指定。
	 * Static class that provides multiple methods for generating cube meshes,
	 * with vertex format specifiable via template parameter.
	 */
	class Cube
	{
	public:
		/**
		 * @brief 生成基础立方体（共享顶点）/ Generate basic cube (shared vertices)
		 *
		 * 使用8个共享顶点生成单位立方体（边长为1，中心在原点）。
		 * Generates a unit cube (side length 1, centered at origin) using 8 shared vertices.
		 *
		 * @tparam V 顶点类型，必须包含 Position 成员 / Vertex type, must contain Position member
		 * @return IndexedTriangleList<V> 索引三角形列表 / Indexed triangle list
		 */
		template<class V>
		static IndexedTriangleList<V> Make()
		{
			namespace dx = DirectX;

			// 边长的一半，使立方体中心在原点 / Half side length, centers cube at origin
			constexpr float side = 1.0f / 2.0f;

			// 8个顶点（共享顶点模式） / 8 vertices (shared vertex mode)
			std::vector<V> vertices(8);
			vertices[0].Position = { -side,-side,-side };
			vertices[1].Position = { side,-side,-side };
			vertices[2].Position = { -side,side,-side };
			vertices[3].Position = { side,side,-side };
			vertices[4].Position = { -side,-side,side };
			vertices[5].Position = { side,-side,side };
			vertices[6].Position = { -side,side,side };
			vertices[7].Position = { side,side,side };

			// 12个三角形（6个面，每个面2个三角形） / 12 triangles (6 faces, 2 triangles per face)
			return{
				std::move(vertices),{
					0,2,1, 2,3,1,   // 前面 / Front face
					1,3,5, 3,7,5,   // 右面 / Right face
					2,6,3, 3,6,7,   // 上面 / Top face
					4,5,7, 4,7,6,   // 后面 / Back face
					0,4,2, 2,4,6,   // 左面 / Left face
					0,1,4, 1,5,4    // 底面 / Bottom face
				}
			};
		}

		/**
		 * @brief 生成带纹理坐标的立方体（蒙皮版本）/ Generate cube with texture coordinates (skinned version)
		 *
		 * 使用14个顶点生成带有完整纹理坐标的立方体，支持立方体贴图展开。
		 * Generates a cube with complete texture coordinates using 14 vertices,
		 * supporting cube map unfolding.
		 *
		 * @tparam V 顶点类型，必须包含 Position 和 TextureCoord 成员
		 *         / Vertex type, must contain Position and TextureCoord members
		 * @return IndexedTriangleList<V> 带纹理坐标的索引三角形列表
		 *         / Indexed triangle list with texture coordinates
		 */
		template<class V>
		static IndexedTriangleList<V> MakeSkinned()
		{
			namespace dx = DirectX;

			// 边长的一半 / Half side length
			constexpr float side = 1.0f / 2.0f;

			// 14个顶点用于展开纹理贴图 / 14 vertices for texture map unfolding
			std::vector<V> vertices(14);

			// 前面顶点 / Front face vertices
			vertices[0].Position = { -side,-side,-side };
			vertices[0].TextureCoord = { 2.0f / 3.0f,0.0f / 4.0f };
			vertices[1].Position = { side,-side,-side };
			vertices[1].TextureCoord = { 1.0f / 3.0f,0.0f / 4.0f };
			vertices[2].Position = { -side,side,-side };
			vertices[2].TextureCoord = { 2.0f / 3.0f,1.0f / 4.0f };
			vertices[3].Position = { side,side,-side };
			vertices[3].TextureCoord = { 1.0f / 3.0f,1.0f / 4.0f };

			// 后面顶点 / Back face vertices
			vertices[4].Position = { -side,-side,side };
			vertices[4].TextureCoord = { 2.0f / 3.0f,3.0f / 4.0f };
			vertices[5].Position = { side,-side,side };
			vertices[5].TextureCoord = { 1.0f / 3.0f,3.0f / 4.0f };
			vertices[6].Position = { -side,side,side };
			vertices[6].TextureCoord = { 2.0f / 3.0f,2.0f / 4.0f };
			vertices[7].Position = { side,side,side };
			vertices[7].TextureCoord = { 1.0f / 3.0f,2.0f / 4.0f };

			// 底部接缝顶点 / Bottom seam vertices
			vertices[8].Position = { -side,-side,-side };
			vertices[8].TextureCoord = { 2.0f / 3.0f,4.0f / 4.0f };
			vertices[9].Position = { side,-side,-side };
			vertices[9].TextureCoord = { 1.0f / 3.0f,4.0f / 4.0f };

			// 左面接缝顶点 / Left seam vertices
			vertices[10].Position = { -side,-side,-side };
			vertices[10].TextureCoord = { 3.0f / 3.0f,1.0f / 4.0f };
			vertices[11].Position = { -side,-side,side };
			vertices[11].TextureCoord = { 3.0f / 3.0f,2.0f / 4.0f };

			// 右面接缝顶点 / Right seam vertices
			vertices[12].Position = { side,-side,-side };
			vertices[12].TextureCoord = { 0.0f / 3.0f,1.0f / 4.0f };
			vertices[13].Position = { side,-side,side };
			vertices[13].TextureCoord = { 0.0f / 3.0f,2.0f / 4.0f };

			return{
				std::move(vertices),{
					0,2,1,   2,3,1,     // 前面 / Front face
					4,8,5,   5,8,9,     // 底面 / Bottom face
					2,6,3,   3,6,7,     // 顶面 / Top face
					4,5,7,   4,7,6,     // 后面 / Back face
					2,10,11, 2,11,6,    // 左面 / Left face
					12,3,7,  12,7,13    // 右面 / Right face
				}
			};
		}

		/**
		 * @brief 生成独立面立方体（每个面独立顶点）/ Generate cube with independent faces
		 *
		 * 使用24个顶点生成立方体，每个面有独立的顶点，便于设置不同的法线。
		 * Generates a cube using 24 vertices, with each face having independent vertices
		 * for easy setup of different normals.
		 *
		 * @tparam V 顶点类型，必须包含 Position 成员 / Vertex type, must contain Position member
		 * @return IndexedTriangleList<V> 独立面索引三角形列表
		 *         / Indexed triangle list with independent faces
		 */
		template<class V>
		static IndexedTriangleList<V> MakeIndependent()
		{
			// 边长的一半 / Half side length
			constexpr float side = 1.0f / 2.0f;

			// 24个顶点（6个面 × 4个顶点）/ 24 vertices (6 faces × 4 vertices)
			std::vector<V> vertices(24);
			vertices[0].Position = { -side,-side,-side };  // 0 前面 / Front face
			vertices[1].Position = { side,-side,-side };   // 1
			vertices[2].Position = { -side,side,-side };   // 2
			vertices[3].Position = { side,side,-side };    // 3
			vertices[4].Position = { -side,-side,side };   // 4 后面 / Back face
			vertices[5].Position = { side,-side,side };    // 5
			vertices[6].Position = { -side,side,side };    // 6
			vertices[7].Position = { side,side,side };     // 7
			vertices[8].Position = { -side,-side,-side };  // 8 左面 / Left face
			vertices[9].Position = { -side,side,-side };   // 9
			vertices[10].Position = { -side,-side,side };  // 10
			vertices[11].Position = { -side,side,side };   // 11
			vertices[12].Position = { side,-side,-side };  // 12 右面 / Right face
			vertices[13].Position = { side,side,-side };   // 13
			vertices[14].Position = { side,-side,side };   // 14
			vertices[15].Position = { side,side,side };    // 15
			vertices[16].Position = { -side,-side,-side }; // 16 底面 / Bottom face
			vertices[17].Position = { side,-side,-side };  // 17
			vertices[18].Position = { -side,-side,side };  // 18
			vertices[19].Position = { side,-side,side };   // 19
			vertices[20].Position = { -side,side,-side };  // 20 顶面 / Top face
			vertices[21].Position = { side,side,-side };   // 21
			vertices[22].Position = { -side,side,side };   // 22
			vertices[23].Position = { side,side,side };    // 23

			return{
				std::move(vertices),{
					0,2, 1,    2,3,1,       // 前面 / Front face
					4,5, 7,    4,7,6,       // 后面 / Back face
					8,10, 9,  10,11,9,      // 左面 / Left face
					12,13,15, 12,15,14,     // 右面 / Right face
					16,17,18, 18,17,19,     // 底面 / Bottom face
					20,23,21, 20,22,23      // 顶面 / Top face
				}
			};
		}
	};
}
