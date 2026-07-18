/**
 * @file LightManager.h
 * @brief 光源管理器 / Light manager
 *
 * 静态光源管理器，负责收集场景中所有点光源和聚光灯，
 * 将它们打包到常量缓冲区中供着色器使用。
 *
 * Static light manager responsible for collecting all point lights and
 * spot lights in the scene, and packing them into constant buffers
 * for use by the shader.
 *
 * @note 这是 DX11 路径的光源管理器。DX12 路径在 MeshRendererSystem
 *       中直接管理灯光状态和常量缓冲区。
 *       This is the DX11 path light manager. The DX12 path manages light
 *       state and constant buffers directly in MeshRendererSystem.
 */
#pragma once
#include "PointLight.h"
#include "SpotLight.h"

namespace YingLong
{
	/**
	 * @brief 光源管理器 / Light manager
	 *
	 * 静态单例模式的光源管理器，维护点光源和聚光灯列表，
	 * 以及对应的像素着色器常量缓冲区。
	 *
	 * Static singleton-style light manager that maintains point light
	 * and spot light lists, along with corresponding pixel shader
	 * constant buffers.
	 */
	class LightManager
	{
	public:
		/**
		 * @brief 初始化光源管理器 / Initialize light manager
		 * @param graphics 图形设备 / Graphics device
		 */
		static void Initialize(Graphics& graphics);

		/**
		 * @brief 提交点光源 / Submit a point light
		 *
		 * 将一个点光源添加到场景的点光源列表中。
		 * Adds a point light to the scene's point light list.
		 *
		 * @param pointLight 点光源 / Point light
		 */
		static void SubmitPointLight(PointLight pointLight);

		/**
		 * @brief 提交聚光灯 / Submit a spot light
		 *
		 * 将一个聚光灯添加到场景的聚光灯列表中。
		 * Adds a spot light to the scene's spot light list.
		 *
		 * @param spotLight 聚光灯 / Spot light
		 */
		static void SubmitSpotLight(SpotLight spotLight);

		/**
		 * @brief 更新光源数据 / Update light data
		 *
		 * 刷新常量缓冲区中的光源数据，供下一帧渲染使用。
		 * Refreshes light data in the constant buffer for the next frame render.
		 *
		 * @param graphics 图形设备 / Graphics device
		 */
		static void Update(Graphics& graphics);

		/**
		 * @brief 获取点光源列表 / Get point light list
		 * @return const std::vector<PointLight>& 点光源列表 / Point light list
		 */
		static const std::vector<PointLight>& GetPointLightList() noexcept;

		/**
		 * @brief 获取点光源常量缓冲区 / Get point light constant buffer
		 * @return PixelConstantBuffer<PointLight::ConstantBuffer> 常量缓冲区 / Constant buffer
		 */
		static PixelConstantBuffer<PointLight::ConstantBuffer> GetPointLightConstantBuffer() noexcept;

		/**
		 * @brief 获取聚光灯列表 / Get spot light list
		 * @return const std::vector<SpotLight>& 聚光灯列表 / Spot light list
		 */
		static const std::vector<SpotLight>& GetSpotLightList() noexcept;

		/**
		 * @brief 获取聚光灯常量缓冲区 / Get spot light constant buffer
		 * @return PixelConstantBuffer<SpotLight::ConstantBuffer> 常量缓冲区 / Constant buffer
		 */
		static PixelConstantBuffer<SpotLight::ConstantBuffer> GetSpotLightConstantBuffer() noexcept;

	private:
		static std::vector<PointLight> pPointLightList;                                    ///< 点光源列表 / Point light list
		static std::vector<SpotLight> pSpotLightList;                                      ///< 聚光灯列表 / Spot light list
		static PixelConstantBuffer<PointLight::ConstantBuffer> pPointLightConstantBuffer;  ///< 点光源常量缓冲区 / Point light constant buffer
		static PixelConstantBuffer<SpotLight::ConstantBuffer> pSpotLightConstantBuffer;    ///< 聚光灯常量缓冲区 / Spot light constant buffer
		
		friend class PointLight;   ///< 友元：PointLight 可以直接访问内部数据 / Friend: PointLight can access internal data directly
		friend class SpotLight;    ///< 友元：SpotLight 可以直接访问内部数据 / Friend: SpotLight can access internal data directly
	};
}
