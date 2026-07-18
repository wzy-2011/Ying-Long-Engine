/**
 * @file ModelImporter.cpp
 * @brief 模型导入器类实现 / Model importer class implementation
 *
 * 实现基于Assimp库的3D模型文件加载功能，支持多线程并行加载。
 * Implements 3D model file loading functionality based on Assimp library,
 * supports multi-threaded parallel loading.
 */
#include "ModelImporter.h"

namespace YingLong
{
	// 移除全局锁以允许完全并行
	// Remove global lock to allow full parallelism

	std::vector<MeshData> ModelImporter::LoadModel(std::string modelFilePath)
	{
		std::vector<MeshData> DataList;
		Assimp::Importer SceneImporter;
		// 使用Assimp读取模型文件：三角化 + 左手系转换 + 生成法线 + 优化 + 翻转UV
		// Read model file using Assimp: triangulate + left-handed + gen normals + optimize + flip UVs
		const aiScene* Scene = SceneImporter.ReadFile(modelFilePath,
			aiProcess_Triangulate           // 三角化所有面 / Triangulate all faces
			| aiProcess_ConvertToLeftHanded // 转换为左手坐标系 / Convert to left-handed
			| aiProcess_GenNormals          // 为缺少法线的模型生成法线 / Generate normals for models missing them
			| aiProcess_JoinIdenticalVertices // 合并重复顶点 / Join identical vertices
			| aiProcess_OptimizeMeshes      // 优化网格（减少绘制调用）/ Optimize meshes (reduce draw calls)
			| aiProcess_FlipUVs             // 翻转UV的V轴 / Flip V-axis of UVs
		);
		if (!Scene || Scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !Scene->mRootNode)
		{
			throw std::runtime_error("[ModelImporter] Failed to load model: " + modelFilePath
				+ " - " + SceneImporter.GetErrorString());
		}

		// 遍历所有网格 / Iterate through all meshes
		for (UINT i = 0; i < Scene->mNumMeshes; i++)
		{
			aiMesh* pMesh = Scene->mMeshes[i];
			MeshData Data;

			// 顶点加载线程：并行加载顶点位置、纹理坐标和法线
			// Vertex loading thread: parallel load vertex positions, texcoords, and normals
			std::thread VerticeLoadingThread([=, &Data]()
				{
					// 预分配顶点内存，避免push_back的线程安全问题
					// Pre-allocate vertex memory to avoid thread safety issues with push_back
					Data.Vertices.resize(pMesh->mNumVertices);

					// 遍历所有顶点（因为是预分配的，直接写入，线程安全）
					// Iterate through all vertices (directly write to pre-allocated, thread-safe)
					for (UINT j = 0; j < pMesh->mNumVertices; j++)
					{
						MeshVertex& Vertex = Data.Vertices[j]; // 直接操作预分配元素 / Directly operate on pre-allocated element

						// 复制顶点位置 / Copy vertex position
						Vertex.Position.x = pMesh->mVertices[j].x;
						Vertex.Position.y = pMesh->mVertices[j].y;
						Vertex.Position.z = pMesh->mVertices[j].z;

						// 复制纹理坐标（如果有）/ Copy texture coordinates (if available)
						if (pMesh->mTextureCoords[0])
						{
							Vertex.TextureCoord.x = pMesh->mTextureCoords[0][j].x;
							Vertex.TextureCoord.y = pMesh->mTextureCoords[0][j].y;
						}
						else
						{
							// 没有纹理坐标时使用默认值 / Use default values when no texture coords
							Vertex.TextureCoord.x = 0.0f;
							Vertex.TextureCoord.y = 0.0f;
						}

						// 复制顶点法线（已由 aiProcess_GenNormals 保证存在，但仍做空指针防护）
						// Copy vertex normal (guaranteed by aiProcess_GenNormals, but still null-safe)
						if (pMesh->mNormals)
						{
							Vertex.Normal.x = pMesh->mNormals[j].x;
							Vertex.Normal.y = pMesh->mNormals[j].y;
							Vertex.Normal.z = pMesh->mNormals[j].z;
						}
						else
						{
							Vertex.Normal = { 0.0f, 1.0f, 0.0f };
						}
					}
				});

			// 索引加载线程：并行加载三角形索引
			// Index loading thread: parallel load triangle indices
			std::thread IndicesLoadingThread([=, &Data]()
				{
					Data.Indices.reserve(pMesh->mNumFaces * 3); // 预分配索引内存 / Pre-allocate index memory
					for (UINT j = 0; j < pMesh->mNumFaces; j++)
					{
						aiFace Face = pMesh->mFaces[j];
						for (UINT q = 0; q < 3; q++)
						{
							Data.Indices.push_back(Face.mIndices[q]);
						}
					}
				});

			// 纹理加载线程：并行加载各种材质纹理
			// Texture loading thread: parallel load various material textures
			std::thread TextureLoadingThread([=, &Data]()
				{
					// 获取模型文件所在目录（先尝试正斜杠，再尝试反斜杠）
					// Get model file directory (try forward slash first, then backslash)
					std::string ModelFileDirectory = modelFilePath.substr(0, modelFilePath.find_last_of('/'));
					if (ModelFileDirectory == modelFilePath)
					{
						ModelFileDirectory = modelFilePath.substr(0, modelFilePath.find_last_of("\\"));
					}
					aiMaterial* mat = Scene->mMaterials[pMesh->mMaterialIndex];
					// 加载各种纹理 / Load various textures
					Data.DiffuseTexture = LoadTexture(mat, aiTextureType_DIFFUSE, ModelFileDirectory);
					Data.NormalTexture = LoadTexture(mat, aiTextureType_NORMALS, ModelFileDirectory);
					Data.RoughnessTexture = LoadTexture(mat, aiTextureType_SHININESS, ModelFileDirectory);
					Data.MetallicTexture = LoadMetallicTexture(mat, ModelFileDirectory);
				});

			// 等待三个加载线程全部完成 / Wait for all three loading threads to complete
			VerticeLoadingThread.join();
			IndicesLoadingThread.join();
			TextureLoadingThread.join();

			DataList.push_back(Data);
		}

		return DataList;
	}

	// 私有LoadTexture和LoadMetallicTexture函数保持不变
	// Private LoadTexture and LoadMetallicTexture functions remain unchanged

	std::shared_ptr<Surface> ModelImporter::LoadTexture(aiMaterial* mat, aiTextureType type, std::string modelDirectory)
	{
		std::shared_ptr<Surface> surface = nullptr;

		// 检查材质中是否有该类型的纹理 / Check if material has this type of texture
		if (mat->GetTextureCount(type))
		{
			aiString Path;
			mat->GetTexture(type, 0, &Path);

			// 解析纹理文件路径（尝试直接路径、相对路径的正斜杠和反斜杠）
			// Resolve texture file path (try direct path, relative path with forward and back slashes)
			std::string TextureFilePath;
			if (std::filesystem::exists(Path.C_Str()))
			{
				TextureFilePath = Path.C_Str();
			}
			else
			{
				TextureFilePath = modelDirectory + "/" + Path.C_Str();
				if (!std::filesystem::exists(TextureFilePath))
				{
					TextureFilePath = modelDirectory + "\\" + Path.C_Str();
				}
			}

			// 创建纹理表面 / Create texture surface
			surface = std::make_unique<Surface>(Surface(TextureFilePath));
		}

		return surface;
	}

	std::shared_ptr<Surface> ModelImporter::LoadMetallicTexture(aiMaterial* mat, std::string modelDirectory)
	{
		std::shared_ptr<Surface> surface = nullptr;

		// 遍历材质所有属性，查找金属度纹理的原始属性
		// Iterate through all material properties, find raw property for metallic texture
		for (UINT i = 0; i < mat->mNumProperties; i++)
		{
			auto& Property = mat->mProperties[i];

			// 只处理字符串类型属性 / Only process string type properties
			if (Property->mType == aiPTI_String)
			{
				std::string Name = Property->mKey.data;
				// 查找ReflectionFactor的文件属性（金属度纹理）
				// Find file property of ReflectionFactor (metallic texture)
				if (Name == "$raw.ReflectionFactor|file")
				{
					UINT StringLength = *(UINT*)Property->mData;
					std::string FilePath = { Property->mData + 4, StringLength };

					// 解析文件路径（同LoadTexture）/ Resolve file path (same as LoadTexture)
					std::string TexturePath = FilePath;
					if (!std::filesystem::exists(FilePath))
					{
						TexturePath = modelDirectory + "/" + FilePath;
						if (!std::filesystem::exists(TexturePath))
						{
							TexturePath = modelDirectory + "\\" + FilePath;
						}
					}

					surface = std::make_shared<Surface>(TexturePath);
				}
			}
		}

		return surface;
	}
}
