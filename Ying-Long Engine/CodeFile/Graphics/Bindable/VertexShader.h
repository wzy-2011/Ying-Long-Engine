/**
 * @file VertexShader.h
 * @brief DX11 顶点着色器头文件 / DX11 vertex shader header file
 *
 * 封装 D3D11 顶点着色器，支持从文件编译并绑定到顶点着色器阶段。
 * Encapsulates D3D11 vertex shader, supporting compilation from file
 * and binding to the vertex shader stage.
 */

#pragma once
#include <string>
#include "Bindable.h"

using namespace Microsoft;

namespace YingLong
{

	/**
	 * @brief DX11 顶点着色器类 / DX11 vertex shader class
	 *
	 * 管理 D3D11 顶点着色器资源，支持从 .hlsl 文件编译着色器，
	 * 创建顶点着色器对象，并绑定到渲染管线的顶点着色器阶段。
	 * 着色器字节码可用于创建输入布局。
	 *
	 * Manages D3D11 vertex shader resources, supporting shader compilation
	 * from .hlsl files, creating vertex shader objects, and binding them
	 * to the vertex shader stage of the rendering pipeline.
	 * Shader bytecode can be used for creating input layouts.
	 */
	class VertexShader : public Bindable
	{
	public:
		/**
		 * @brief 默认构造函数 / Default constructor
		 */
		VertexShader() = default;

		/**
		 * @brief 构造函数 / Constructor
		 * @param graphics 图形设备引用 / Graphics device reference
		 * @param filePath 着色器文件路径 / Shader file path
		 *
		 * 从指定文件加载并编译顶点着色器。
		 * Loads and compiles a vertex shader from the specified file.
		 */
		VertexShader(Graphics& graphics, const std::string& filePath);

		/**
		 * @brief 析构函数 / Destructor
		 */
		~VertexShader() = default;

		/**
		 * @brief 初始化顶点着色器 / Initialize vertex shader
		 * @param graphics 图形设备引用 / Graphics device reference
		 * @param filePath 着色器文件路径 / Shader file path
		 *
		 * 从文件编译并创建顶点着色器对象。编译失败时抛出异常。
		 * Compiles and creates a vertex shader object from file.
		 * Throws an exception on compilation failure.
		 */
		void Initialize(Graphics& graphics, const std::string& filePath);

		/**
		 * @brief 绑定顶点着色器 / Bind vertex shader
		 * @param graphics 图形设备引用 / Graphics device reference
		 *
		 * 将顶点着色器绑定到顶点着色器阶段。
		 * Binds the vertex shader to the vertex shader stage.
		 */
		void Bind(Graphics& graphics) noexcept override;

		/**
		 * @brief 获取着色器字节码 / Get shader bytecode
		 * @return 指向着色器字节码的 ID3D10Blob 指针 / Pointer to shader bytecode ID3D10Blob
		 *
		 * 返回着色器编译后的字节码，用于创建输入布局等操作。
		 * Returns the compiled shader bytecode, used for creating input layouts, etc.
		 */
		ID3D10Blob* GetBytecode();

	private:
		WRL::ComPtr<ID3D11VertexShader> pVertexShaderObject;   ///< D3D11 顶点着色器对象 / D3D11 vertex shader object
		WRL::ComPtr<ID3D10Blob> pVertexShaderBuffer;            ///< 顶点着色器字节码缓冲区 / Vertex shader bytecode buffer
	};
}
