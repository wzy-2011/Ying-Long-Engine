/**
 * @file Texture.h
 * @brief DX11 纹理头文件 / DX11 texture header file
 *
 * 封装 D3D11 2D 纹理和着色器资源视图，用于在着色器中采样纹理数据。
 * Encapsulates D3D11 2D texture and shader resource view, used for
 * sampling texture data in shaders.
 */

#pragma once
#include "../Surface/Surface.h"
#include "Bindable.h"

namespace YingLong
{
	/**
	 * @brief DX11 2D 纹理类 / DX11 2D texture class
	 *
	 * 管理 D3D11 2D 纹理资源及其着色器资源视图，支持从 Surface 对象
	 * 创建纹理，并绑定到像素着色器的指定纹理槽。
	 * Manages D3D11 2D texture resources and their shader resource views,
	 * supporting texture creation from Surface objects and binding to
	 * specified texture slots of the pixel shader.
	 */
	class Texture : public Bindable
	{
	public:
		/**
		 * @brief 默认构造函数 / Default constructor
		 */
		Texture() = default;

		/**
		 * @brief 构造函数 / Constructor
		 * @param graphics 图形设备引用 / Graphics device reference
		 * @param surface 表面对象，包含纹理像素数据 / Surface object containing texture pixel data
		 * @param index 纹理槽位索引 / Texture slot index (default: 0)
		 *
		 * 从 Surface 对象创建 2D 纹理及其着色器资源视图。
		 * Creates a 2D texture and its shader resource view from a Surface object.
		 */
		Texture(Graphics& graphics, const Surface& surface, UINT index = 0);

		/**
		 * @brief 拷贝构造函数 / Copy constructor
		 * @param other 源纹理对象 / Source texture object
		 */
		Texture(const Texture& other);

		/**
		 * @brief 初始化纹理 / Initialize texture
		 * @param graphics 图形设备引用 / Graphics device reference
		 * @param surface 表面对象，包含纹理像素数据 / Surface object containing texture pixel data
		 * @param index 纹理槽位索引 / Texture slot index (default: 0)
		 *
		 * 从 Surface 对象创建 2D 纹理及其着色器资源视图。
		 * Creates a 2D texture and its shader resource view from a Surface object.
		 */
		void Initialize(Graphics& graphics, const Surface& surface, UINT index = 0);

		/**
		 * @brief 绑定纹理 / Bind texture
		 * @param graphics 图形设备引用 / Graphics device reference
		 *
		 * 将纹理资源视图绑定到像素着色器的指定纹理槽。
		 * Binds the texture resource view to the specified pixel shader texture slot.
		 */
		void Bind(Graphics& graphics) noexcept override;

	private:
		UINT TextureIndex;                                          ///< 纹理绑定槽位索引 / Texture bind slot index
		WRL::ComPtr<ID3D11Texture2D> pTextureObject;                ///< D3D11 2D 纹理对象 / D3D11 2D texture object
		WRL::ComPtr<ID3D11ShaderResourceView> pTextureResource;     ///< 纹理着色器资源视图 / Texture shader resource view
	};
}
