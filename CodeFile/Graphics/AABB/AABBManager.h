/**
 * @file AABBManager.h
 * @brief AABB管理器类头文件 / AABB manager class header file
 *
 * 管理多个轴对齐包围盒的管理器类（目前为骨架代码）。
 * Manager class for managing multiple axis-aligned bounding boxes
 * (currently skeleton code).
 */
#pragma once
#include "AABB.h"

namespace YingLong
{
	/**
	 * @brief AABB管理器类 / AABB manager class
	 *
	 * 用于管理和组织多个AABB包围盒的管理器类。
	 * Manager class for managing and organizing multiple AABB bounding boxes.
	 *
	 * @note 目前为骨架代码，功能待完善 / Currently skeleton code, functionality pending
	 */
	class AABBManager
	{
	public:
		/**
		 * @brief 添加一个AABB包围盒 / Add an AABB bounding box
		 *
		 * @param aabb 要添加的AABB对象 / AABB object to add
		 */
		static void AddAABB(AABB aabb);

	private:
		// 待实现成员 / Members to be implemented
	};
}
