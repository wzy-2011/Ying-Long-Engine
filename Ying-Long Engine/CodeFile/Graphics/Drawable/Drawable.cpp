/** @file Drawable.cpp
 *  @brief 可绘制对象基类实现（已弃用） - Drawable base class implementation (deprecated)
 *
 *  包含 Drawable 基类的成员函数实现。
 *  Contains the member function implementations of the Drawable base class.
 */
#include "Drawable.h"
#include "../Graphics.h"

namespace YingLong
{
	/** @brief 拷贝构造函数
	 *  Copy constructor
	 *
	 *  @param 要拷贝的可绘制对象 / The drawable to copy from
	 */
	Drawable::Drawable(const Drawable&)
	{

	}

	/** @brief 绘制可绘制对象
	 *  Draw the drawable object
	 *
	 *  依次绑定实例级绑定和静态级绑定，然后执行索引绘制。
	 *  Binds instance-level and static-level bindings sequentially, then
	 *  performs indexed drawing.
	 *
	 *  @param graphics 图形设备对象引用 / Graphics device object reference
	 */
	void Drawable::Draw(Graphics& graphics) const noexcept
	{
		// 先绑定实例级资源（每个实例独有的）
		// First bind instance-level resources (unique per instance)
		for (auto& b : binds)
		{
			b->Bind(graphics);
		}
		// 再绑定静态级资源（同类实例共享的）
		// Then bind static-level resources (shared by instances of same type)
		for (auto& b : GetStaticBinds())
		{
			b->Bind(graphics);
		}
		// 执行索引绘制
		// Perform indexed drawing
		graphics.DrawIndexed(pIndexBuffer->GetCount());
	}

	/** @brief 添加实例级绑定对象
	 *  Add an instance-level bindable object
	 *
	 *  将可绑定对象添加到实例绑定列表中。
	 *  注意：索引缓冲区必须使用 AddIndexBuffer 单独添加。
	 *
	 *  Adds a bindable object to the instance binding list.
	 *  Note: Index buffers must be added separately using AddIndexBuffer.
	 *
	 *  @param bind 要添加的可绑定对象的唯一指针 / Unique pointer to the bindable to add
	 */
	void Drawable::AddBind(std::unique_ptr<Bindable> bind) noexcept
	{
		// 确保不通过此方法添加索引缓冲区
		// Ensure index buffer is not added through this method
		assert("Must use AddIndexBuffer to bind index buffer!"
			&& typeid(*bind) != typeid(IndexBuffer));
		binds.push_back(std::move(bind));
	}

	/** @brief 添加实例级索引缓冲区
	 *  Add an instance-level index buffer
	 *
	 *  将索引缓冲区添加到实例绑定列表中，并保存其指针以便绘制时使用。
	 *  Adds the index buffer to the instance binding list and saves its
	 *  pointer for use during drawing.
	 *
	 *  @param IndexBufferObject 索引缓冲区对象的唯一指针 / Unique pointer to the index buffer object
	 */
	void Drawable::AddIndexBuffer(std::unique_ptr<class IndexBuffer> IndexBufferObject) noexcept
	{
		// 确保只添加一次索引缓冲区
		// Ensure index buffer is only added once
		assert("Attempting to add index buffer a second time!"
			&& pIndexBuffer == nullptr);
		pIndexBuffer = IndexBufferObject.get();
		binds.push_back(std::move(IndexBufferObject));
	}
}
