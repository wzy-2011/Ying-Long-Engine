/**
 * @file ModelImporter.h
 * @brief 模型导入器类 / Model importer class
 *
 * 使用Assimp库导入3D模型文件，支持多线程加载顶点、索引和纹理。
 * Imports 3D model files using Assimp library, supports multi-threaded loading
 * of vertices, indices, and textures.
 */
#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <Windows.h>
#include <exception>
#include <memory>
#include <thread>
#include <mutex>
#include "../Bindable/Texture.h"

using namespace DirectX;

namespace YingLong
{
	/**
	 * @brief 网格顶点结构体 / Mesh vertex structure
	 *
	 * 存储单个顶点的位置、法线和纹理坐标数据。
	 * Stores position, normal, and texture coordinate data for a single vertex.
	 */
	struct MeshVertex
	{
		/**
		 * @brief 默认构造函数 / Default constructor
		 */
		MeshVertex() = default;

		/**
		 * @brief 拷贝构造函数 / Copy constructor
		 *
		 * @param other 另一个MeshVertex对象 / Another MeshVertex object
		 */
		MeshVertex(const MeshVertex& other)
		{
			this->Position = other.Position;
			this->Normal = other.Normal;
			this->TextureCoord = other.TextureCoord;
		}

		XMFLOAT3 Position;       ///< 顶点位置坐标 / Vertex position coordinates
		XMFLOAT3 Normal;         ///< 顶点法线向量 / Vertex normal vector
		XMFLOAT2 TextureCoord;   ///< 顶点纹理坐标 / Vertex texture coordinates
	};
	
	/**
	 * @brief 网格数据结构体 / Mesh data structure
	 *
	 * 存储一个网格的完整数据，包括顶点、索引和各种纹理。
	 * Stores complete data for a mesh, including vertices, indices, and various textures.
	 */
	struct MeshData
	{
		/**
		 * @brief 默认构造函数 / Default constructor
		 */
		MeshData() = default;

		/**
		 * @brief 拷贝构造函数 / Copy constructor
		 */
		MeshData(const MeshData&) = default;

		std::vector<MeshVertex> Vertices;              ///< 顶点数组 / Vertex array
		std::vector<unsigned int> Indices;             ///< 索引数组（32位，支持大型模型）/ Index array (32-bit, supports large models)

		std::shared_ptr<Surface> DiffuseTexture;       ///< 漫反射纹理 / Diffuse texture
		std::shared_ptr<Surface> MetallicTexture;      ///< 金属度纹理 / Metallic texture
		std::shared_ptr<Surface> NormalTexture;        ///< 法线纹理 / Normal texture
		std::shared_ptr<Surface> RoughnessTexture;     ///< 粗糙度纹理 / Roughness texture
	};

	/**
	 * @brief 模型导入器类 / Model importer class
	 *
	 * 静态类，使用Assimp库从文件导入3D模型，支持多线程并行加载。
	 * Static class that imports 3D models from files using Assimp library,
	 * supports multi-threaded parallel loading.
	 */
	class ModelImporter
	{
	public:
		/**
		 * @brief 从文件加载模型 / Load model from file
		 *
		 * 使用Assimp库加载3D模型文件，自动三角化并转换为左手坐标系。
		 * 加载过程使用多线程并行处理顶点、索引和纹理。
		 * Loads 3D model file using Assimp library, automatically triangulates
		 * and converts to left-handed coordinate system. Loading process uses
		 * multi-threaded parallel processing for vertices, indices, and textures.
		 *
		 * @param modelFilePath 模型文件路径 / Model file path
		 * @return std::vector<MeshData> 网格数据列表 / Mesh data list
		 */
		static std::vector<MeshData> LoadModel(std::string modelFilePath);

	private:
		/**
		 * @brief 从材质中加载指定类型的纹理 / Load texture of specified type from material
		 *
		 * @param mat Assimp材质指针 / Assimp material pointer
		 * @param type 纹理类型 / Texture type
		 * @param modelDirectory 模型文件所在目录 / Model file directory
		 * @return std::shared_ptr<Surface> 纹理表面指针（失败返回nullptr）
		 *         / Texture surface pointer (returns nullptr on failure)
		 */
		static std::shared_ptr<Surface> LoadTexture(aiMaterial* mat, aiTextureType type, std::string modelDirectory);

		/**
		 * @brief 从材质中加载金属度纹理 / Load metallic texture from material
		 *
		 * 通过读取材质的原始属性来查找金属度纹理。
		 * Finds metallic texture by reading raw material properties.
		 *
		 * @param mat Assimp材质指针 / Assimp material pointer
		 * @param modelDirectory 模型文件所在目录 / Model file directory
		 * @return std::shared_ptr<Surface> 金属度纹理指针（失败返回nullptr）
		 *         / Metallic texture pointer (returns nullptr on failure)
		 */
		static std::shared_ptr<Surface> LoadMetallicTexture(aiMaterial* mat, std::string modelDirectory);
	};
}
