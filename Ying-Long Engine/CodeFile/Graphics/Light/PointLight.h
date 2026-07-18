/**
 * @file PointLight.h
 * @brief 点光源类 / Point light class
 *
 * 表示 3D 场景中的点光源。包含位置、颜色、强度等属性，
 * 使用 SolidSphereDrawable 可视化光源位置，支持 ImGui 控制面板
 * 和 YAML 序列化/反序列化。
 *
 * Represents a point light in the 3D scene. Contains position, color,
 * intensity and other properties, uses SolidSphereDrawable to visualize
 * the light position, supports ImGui control panel and YAML
 * serialization/deserialization.
 *
 * @note 这是 DX11 路径的点光源类。DX12 路径使用独立的
 *       DX12PointLightState/DX12PointLightCB。
 *       This is the DX11 path point light class. The DX12 path uses
 *       independent DX12PointLightState/DX12PointLightCB.
 */
#pragma once
#include <filesystem>
#include "../../../yaml-cpp/include/yaml-cpp/yaml.h"
#include <fstream>
#include <sstream>
#include "../Graphics.h"
#include "../Bindable/ConstantBuffers.h"
#include "../Drawable/SolidSphere.h"

namespace YingLong
{
	/**
	 * @brief 点光源类 / Point light class
	 *
	 * 点光源从一个点向所有方向发射光线。
	 * Point lights emit light from a single point in all directions.
	 */
	class PointLight
	{
	public:
		/**
		 * @brief 点光源数据结构体 / Point light data structure
		 *
		 * 布局需与着色器中的 cbuffer 匹配，注意 16 字节对齐填充。
		 * Layout must match the cbuffer in the shader, note 16-byte alignment padding.
		 */
		struct Data
		{
			Data()
			{
				this->Position = { 0.0f, 0.0f, 0.0f };
				this->padding0 = 0.0f;
				this->Color = { 1.0f, 1.0f, 1.0f };
				this->Intensity = 1000.0f;
			}

		public:
			XMFLOAT3 Position;  ///< 光源位置 / Light position
		private:
			float padding0;     ///< 16 字节对齐填充 / 16-byte alignment padding
		public:
			XMFLOAT3 Color;     ///< 光源颜色 / Light color
			float Intensity;    ///< 光强 / Light intensity
		};

		/**
		 * @brief 点光源常量缓冲区 / Point light constant buffer
		 *
		 * 包含最多 50 个点光源的数据，用于着色器计算光照。
		 * Contains data for up to 50 point lights, used by the shader
		 * to compute lighting.
		 */
		struct ConstantBuffer
		{
		public:
			Data PointLightList[50];    ///< 点光源数组 / Point light array
			int PointLightCount;        ///< 点光源数量 / Point light count
			XMFLOAT3 CameraPosition;    ///< 相机位置（用于镜面反射计算） / Camera position (for specular calculation)
		};

		/**
		 * @brief 拷贝构造函数 / Copy constructor
		 * @param other 源对象 / Source object
		 */
		PointLight(const PointLight&);

		/**
		 * @brief 构造函数 / Constructor
		 * @param graphics 图形设备 / Graphics device
		 * @param radius 可视化球体半径 / Visualization sphere radius
		 */
		PointLight(Graphics& graphics, float radius = 0.15f);

		/**
		 * @brief 生成 ImGui 控制面板 / Spawn ImGui control window
		 * @param PointLightName 面板名称 / Panel name
		 */
		void SpawnControlWindow(const char* PointLightName) noexcept;

		/**
		 * @brief 绘制光源可视化 / Draw light visualization
		 * @param graphics 图形设备 / Graphics device
		 */
		void Draw(Graphics& graphics) const noexcept;

		/**
		 * @brief 绑定光源数据到渲染管线 / Bind light data to render pipeline
		 */
		void Bind() const noexcept;

		/**
		 * @brief 重置光源到默认状态 / Reset light to default state
		 */
		void Reset() noexcept;

		/**
		 * @brief 序列化光源数据 / Serialize light data
		 * @param filePath 文件路径 / File path
		 */
		void Serialize(const std::string& filePath) const;

		/**
		 * @brief 反序列化光源数据 / Deserialize light data
		 * @param filePath 文件路径 / File path
		 */
		void Deserialize(const std::string& filePath);

		/**
		 * @brief 保存光源数据（热键触发） / Save light data (hotkey triggered)
		 * @param filePath 文件路径 / File path
		 */
		void Save(const std::string filePath) const noexcept;

		/**
		 * @brief 导入光源数据 / Import light data
		 * @param filePath 文件路径 / File path
		 */
		void Import(const std::string& filePath) noexcept;

		Data LightData;  ///< 光源数据 / Light data

	private:
		mutable SolidSphereDrawable mesh;  ///< 可视化网格 / Visualization mesh
	};
}
