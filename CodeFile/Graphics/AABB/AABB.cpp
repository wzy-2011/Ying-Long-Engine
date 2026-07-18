/**
 * @file AABB.cpp
 * @brief 轴对齐包围盒类实现 / Axis-Aligned Bounding Box class implementation
 *
 * 实现AABB的构造函数和射线相交检测功能。
 * Implements AABB constructors and ray intersection detection functionality.
 */
#include "AABB.h"

namespace YingLong
{
	AABB::AABB()
	{
		// 初始化为无效包围盒：Max为负无穷，Min为正无穷
		// Initialize to invalid bounding box: Max is -infinity, Min is +infinity
		this->Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
		this->Min = { FLT_MAX, FLT_MAX, FLT_MAX };
	}

	AABB::AABB(XMFLOAT3 max, XMFLOAT3 min)
	{
		// 使用setter方法设置最大和最小点
		// Use setter methods to set max and min points
		this->SetMax(max);
		this->SetMin(min);
	}

	AABB::AABB(const AABB& other)
	{
		// 拷贝最大和最小点坐标
		// Copy max and min point coordinates
		this->Max = other.Max;
		this->Min = other.Min;
	}

	void AABB::SetMin(XMFLOAT3 min) noexcept
	{
		this->Min = min;
	}

	void AABB::SetMax(XMFLOAT3 max) noexcept
	{
		this->Max = max;
	}

	bool AABB::Intersect(XMFLOAT3 rayOrigin, XMFLOAT3 rayDirection, float* distance)
	{
		// 计算射线方向的倒数（避免重复除法）
		// Calculate reciprocal of ray direction (avoids repeated division)
		XMFLOAT3 dirfrac = {};
		dirfrac.x = 1.0f / rayDirection.x;
		dirfrac.y = 1.0f / rayDirection.y;
		dirfrac.z = 1.0f / rayDirection.z;

		// 计算射线与三个轴对齐slab的相交距离
		// Calculate intersection distances with three axis-aligned slabs
		float t1 = (this->Min.x - rayOrigin.x) * dirfrac.x;
		float t2 = (this->Max.x - rayOrigin.x) * dirfrac.x;
		float t3 = (this->Min.y - rayOrigin.y) * dirfrac.y;
		float t4 = (this->Max.y - rayOrigin.y) * dirfrac.y;
		float t5 = (this->Min.z - rayOrigin.z) * dirfrac.z;
		float t6 = (this->Max.z - rayOrigin.z) * dirfrac.z;

		// tmin = 进入三个slab的最远距离（射线进入包围盒的距离）
		// tmin = farthest distance entering three slabs (distance ray enters box)
		float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
		// tmax = 离开三个slab的最近距离（射线离开包围盒的距离）
		// tmax = nearest distance leaving three slabs (distance ray exits box)
		float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

		// 如果tmax < 0，说明包围盒在射线的反方向，不相交
		// If tmax < 0, box is behind ray origin, no intersection
		if (tmax < 0)
		{
			if (distance)
			{
				*distance = tmax;
			}
			return false;
		}

		// 如果tmin > tmax，说明射线不穿过包围盒
		// If tmin > tmax, ray does not pass through the box
		if (tmin > tmax)
		{
			if (distance)
			{
				*distance = tmax;
			}
			return false;
		}

		// 相交，返回进入距离tmin
		// Intersection occurs, return entry distance tmin
		if (distance)
		{
			*distance = tmin;
		}
		return true;
	}
}
