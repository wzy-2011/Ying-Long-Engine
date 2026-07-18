/**
 * @file Ray.cpp
 * @brief 射线类实现 / Ray class implementation
 *
 * 实现射线的构造、屏幕坐标转换和相交检测功能。
 * Implements ray construction, screen coordinate conversion,
 * and intersection detection functionality.
 */
#include "Ray.h"

namespace YingLong
{
	Ray::Ray(const XMFLOAT3& origin, const XMFLOAT3& direction)
	{
		// 使用setter方法设置起点和方向
		// Use setter methods to set origin and direction
		this->SetOrigin(origin);
		this->SetDirection(direction);
	}

	Ray::Ray(const Camera& camera, XMFLOAT2 screenSize, const XMFLOAT2& mousePosition)
	{
		// 从屏幕坐标生成拾取射线
		// Generate picking ray from screen coordinates
		this->ScreenToRay(camera, screenSize, mousePosition);
	}

	void Ray::ScreenToRay(const Camera& camera, XMFLOAT2 screenSize, const XMFLOAT2& mousePosition)
	{
		// D向量：用于将NDC坐标从[0,1]转换到[-1,1]范围，并翻转Y轴
		// D vector: used to convert NDC from [0,1] to [-1,1] range and flip Y axis
		static const XMVECTORF32 D = { {{-1.0f, 1.0f, 0.0f, 0.0f}} };
		// 将鼠标坐标限制在屏幕范围内 / Clamp mouse coordinates to screen bounds
		XMVECTOR V = XMVectorSet(std::clamp(mousePosition.x, 0.0f, screenSize.x - 1.0f), std::clamp(mousePosition.y, 0.0f, screenSize.y - 1.0f), 0.0f, 1.0f);
		
		// 计算缩放因子：将屏幕像素坐标转换为NDC坐标
		// Calculate scale factor: convert screen pixel coordinates to NDC coordinates
		XMVECTOR Scale = XMVectorSet(screenSize.x * 0.5f, -screenSize.y * 0.5f, camera.GetFarZ() - camera.GetNearZ(), 1.0f);
		Scale = XMVectorReciprocal(Scale);

		// 计算偏移量 / Calculate offset
		XMVECTOR Offset = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
		Offset = XMVectorMultiplyAdd(Scale, Offset, D.v);

		// 计算视图投影矩阵的逆矩阵
		// Calculate inverse of view-projection matrix
		XMMATRIX Transform = XMMatrixMultiply(camera.GetMatrix(), camera.GetProjection());
		Transform = XMMatrixInverse(nullptr, Transform);

		// 将屏幕坐标转换到世界空间
		// Convert screen coordinates to world space
		XMVECTOR Target = XMVectorMultiplyAdd(V, Scale, Offset);
		Target = XMVector3TransformCoord(Target, Transform);

		// 计算射线方向：从相机位置指向目标点，并归一化
		// Calculate ray direction: from camera position to target point, and normalize
		XMFLOAT3 direction;
		XMVECTOR CameraPosition = XMVectorSet(camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z, 1.0f);
		XMStoreFloat3(&direction, XMVector3Normalize(Target - CameraPosition));

		// 设置射线方向和起点 / Set ray direction and origin
		this->SetDirection(direction);
		this->SetOrigin(camera.GetPosition());
	}

	void Ray::SetOrigin(const XMFLOAT3& origin)
	{
		this->Origin = origin;
	}

	void Ray::SetDirection(const XMFLOAT3& direction)
	{
		// 计算方向向量的长度 / Calculate length of direction vector
		XMVECTOR dirLength = XMVector3Length(XMLoadFloat3(&direction));
		// 检查是否接近单位长度（允许小误差）/ Check if close to unit length (allow small error)
		XMVECTOR error = XMVectorAbs(dirLength - XMVectorSplatOne());
		assert(XMVector3Less(error, XMVectorReplicate(1e-5f)));

		// 归一化方向向量并存储 / Normalize direction vector and store
		XMStoreFloat3(&this->Direction, XMVector3Normalize(XMLoadFloat3(&direction)));
	}

	bool Ray::Hit(const BoundingBox& boundingBox, float* distance, float maxDistance)
	{
		float dist;
		// 使用DirectXCollision的相交检测函数
		// Use DirectXCollision intersection detection function
		bool res = boundingBox.Intersects(XMLoadFloat3(&this->Origin), XMLoadFloat3(&this->Direction), dist);
		if (distance)
			*distance = dist;
		// 如果相交距离超过最大距离，则视为不相交
		// If intersection distance exceeds max distance, consider no intersection
		return dist > maxDistance ? false : res;
	}

	const XMFLOAT3& Ray::GetOrigin() const noexcept
	{
		return this->Origin;
	}

	const XMFLOAT3& Ray::GetDirection() const noexcept
	{
		return this->Direction;
	}
}
