/**
 * @file Sampler.h
 * @brief DX11 采样器状态头文件 / DX11 sampler state header file
 *
 * 封装 D3D11 采样器状态，用于控制纹理采样方式。
 * Encapsulates D3D11 sampler state, used to control texture sampling methods.
 */

#pragma once
#include "Bindable.h"

namespace YingLong
{
	/**
	 * @brief DX11 采样器状态类 / DX11 sampler state class
	 *
	 * 管理 D3D11 采样器状态对象，控制纹理在像素着色器中的采样方式，
	 * 包括过滤模式、寻址模式等。默认使用线性过滤和环绕寻址。
	 * Manages D3D11 sampler state objects, controlling how textures are
	 * sampled in pixel shaders, including filter mode, addressing mode, etc.
	 * Defaults to linear filtering and wrap addressing.
	 */
	class Sampler : public Bindable
	{
	public:
		/**
		 * @brief 默认构造函数 / Default constructor
		 */
		Sampler();

		/**
		 * @brief 构造函数（立即初始化）/ Constructor (immediate initialization)
		 * @param graphics 图形设备引用 / Graphics device reference
		 *
		 * 创建并初始化采样器状态。
		 * Creates and initializes the sampler state.
		 */
		Sampler(Graphics& graphics);

		/**
		 * @brief 拷贝构造函数 / Copy constructor
		 * @param other 源采样器对象 / Source sampler object
		 */
		Sampler(const Sampler& other);

		/**
		 * @brief 初始化采样器 / Initialize sampler
		 * @param graphics 图形设备引用 / Graphics device reference
		 *
		 * 创建使用线性过滤和环绕寻址的采样器状态。
		 * Creates sampler state with linear filtering and wrap addressing.
		 */
		void InitializeSampler(Graphics& graphics);

		/**
		 * @brief 绑定采样器 / Bind sampler
		 * @param graphics 图形设备引用 / Graphics device reference
		 *
		 * 将采样器状态绑定到像素着色器的第 0 个采样槽。
		 * Binds the sampler state to pixel shader sampler slot 0.
		 */
		void Bind(Graphics& graphics) noexcept override;

	private:
		WRL::ComPtr<ID3D11SamplerState> SamplerState;   ///< D3D11 采样器状态对象 / D3D11 sampler state object
	};
}
