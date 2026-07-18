/**
 * @file PixelShader.h
 * @brief DX11 像素着色器头文件 / DX11 pixel shader header file
 *
 * 封装 D3D11 像素着色器，支持从文件编译并绑定到像素着色器阶段。
 * Encapsulates D3D11 pixel shader, supporting compilation from file
 * and binding to the pixel shader stage.
 */

#pragma once
#include <string>
#include "Bindable.h"

using namespace Microsoft;

namespace YingLong
{

	/**
	 * @brief DX11 像素着色器类 / DX11 pixel shader class
	 *
	 * 管理 D3D11 像素着色器资源，支持从 .hlsl 文件编译着色器，
	 * 创建像素着色器对象，并绑定到渲染管线的像素着色器阶段。
	 * Manages D3D11 pixel shader resources, supporting shader compilation
	 * from .hlsl files, creating pixel shader objects, and binding them
	 * to the pixel shader stage of the rendering pipeline.
	 */
	class PixelShader : public Bindable
	{
	public:
		/**
		 * @brief 默认构造函数 / Default constructor
		 */
		PixelShader() = default;

		/**
		 * @brief 构造函数 / Constructor
		 * @param graphics 图形设备引用 / Graphics device reference
		 * @param FilePath 着色器文件路径 / Shader file path
		 *
		 * 从指定文件加载并编译像素着色器。
		 * Loads and compiles a pixel shader from the specified file.
		 */
		PixelShader(Graphics& graphics, const std::string& FilePath);

		/**
		 * @brief 析构函数 / Destructor
		 */
		~PixelShader() = default;

		/**
		 * @brief 初始化像素着色器 / Initialize pixel shader
		 * @param graphics 图形设备引用 / Graphics device reference
		 * @param FilePath 着色器文件路径 / Shader file path
		 *
		 * 从文件编译并创建像素着色器对象。
		 * Compiles and creates a pixel shader object from file.
		 */
		void Initialize(Graphics& graphics, const std::string& FilePath);

		/**
		 * @brief 绑定像素着色器 / Bind pixel shader
		 * @param graphics 图形设备引用 / Graphics device reference
		 *
		 * 将像素着色器绑定到像素着色器阶段。
		 * Binds the pixel shader to the pixel shader stage.
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
		WRL::ComPtr<ID3D11PixelShader> pPixelShaderObject;   ///< D3D11 像素着色器对象 / D3D11 pixel shader object
		WRL::ComPtr<ID3D10Blob> pPixelShaderBuffer;          ///< 像素着色器字节码缓冲区 / Pixel shader bytecode buffer
	};
}
