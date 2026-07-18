/**
 * @file Ray.h
 * @brief 射线类 / Ray class
 *
 * 提供三维射线的表示及与包围盒的相交检测功能，
 * 支持从屏幕坐标和相机生成拾取射线。
 * Provides 3D ray representation and bounding box intersection detection,
 * supports generating picking rays from screen coordinates and camera.
 */
#pragma once
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include "../Camera/Camera.h"
using namespace DirectX;

namespace YingLong
{
	/**
	 * @brief 射线类 / Ray class
	 *
	 * 表示三维空间中的射线，由起点和方向向量组成。
	 * 支持从屏幕坐标生成拾取射线，以及与包围盒的相交检测。
	 * Represents a ray in 3D space, consisting of origin and direction vector.
	 * Supports generating picking rays from screen coordinates and
	 * bounding box intersection detection.
	 */
	class Ray
	{
	public:
		/**
		 * @brief 默认构造函数 / Default constructor
		 */
		Ray() = default;

		/**
		 * @brief 用起点和方向构造射线 / Construct ray with origin and direction
		 *
		 * @param origin 射线起点 / Ray origin
		 * @param direction 射线方向（应该是归一化的）/ Ray direction (should be normalized)
		 */
		Ray(const XMFLOAT3& origin, const XMFLOAT3& direction);

		/**
		 * @brief 从相机和屏幕坐标构造拾取射线
		 *        Construct picking ray from camera and screen coordinates
		 *
		 * @param camera 相机对象 / Camera object
		 * @param screenSize 屏幕尺寸 / Screen size
		 * @param mousePosition 鼠标屏幕坐标 / Mouse screen coordinates
		 */
		Ray(const Camera& camera, XMFLOAT2 screenSize, const XMFLOAT2& mousePosition);

		/**
		 * @brief 析构函数 / Destructor
		 */
		~Ray() = default;

		/**
		 * @brief 从屏幕坐标生成拾取射线 / Generate picking ray from screen coordinates
		 *
		 * 将屏幕鼠标坐标转换为3D空间中的拾取射线。
		 * Converts screen mouse coordinates to a picking ray in 3D space.
		 *
		 * @param camera 相机对象 / Camera object
		 * @param screenSize 屏幕尺寸 / Screen size
		 * @param mousePosition 鼠标屏幕坐标 / Mouse screen coordinates
		 */
		void ScreenToRay(const Camera& camera, XMFLOAT2 screenSize, const XMFLOAT2& mousePosition);

		/**
		 * @brief 设置射线起点 / Set ray origin
		 *
		 * @param origin 射线起点 / Ray origin
		 */
		void SetOrigin(const XMFLOAT3& origin);

		/**
		 * @brief 设置射线方向 / Set ray direction
		 *
		 * 方向向量将被自动归一化，并验证是否接近单位长度。
		 * Direction vector will be automatically normalized and verified
		 * to be close to unit length.
		 *
		 * @param direction 射线方向 / Ray direction
		 */
		void SetDirection(const XMFLOAT3& direction);

		/**
		 * @brief 检测射线与包围盒是否相交
		 *        Check if ray intersects with bounding box
		 *
		 * 使用DirectXCollision的BoundingBox相交检测。
		 * Uses DirectXCollision BoundingBox intersection detection.
		 *
		 * @param boundingBox 轴对齐包围盒 / Axis-aligned bounding box
		 * @param distance 输出相交距离（可选，为nullptr时不输出）
		 *        / Output intersection distance (optional, not output when nullptr)
		 * @param maxDistance 最大检测距离 / Maximum detection distance
		 * @return bool 是否相交 / Whether intersection occurs
		 */
		bool Hit(const BoundingBox& boundingBox, float* distance = nullptr, float maxDistance = FLT_MAX);

		/**
		 * @brief 获取射线起点 / Get ray origin
		 *
		 * @return const XMFLOAT3& 射线起点引用 / Ray origin reference
		 */
		const XMFLOAT3& GetOrigin() const noexcept;

		/**
		 * @brief 获取射线方向 / Get ray direction
		 *
		 * @return const XMFLOAT3& 射线方向引用 / Ray direction reference
		 */
		const XMFLOAT3& GetDirection() const noexcept;

	private:
		XMFLOAT3 Origin;       ///< 射线起点 / Ray origin
		XMFLOAT3 Direction;    ///< 射线方向（归一化）/ Ray direction (normalized)
	};
}
