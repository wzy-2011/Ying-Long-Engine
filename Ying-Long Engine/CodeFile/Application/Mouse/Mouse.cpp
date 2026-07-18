/**
 * @file Mouse.cpp
 * @brief 鼠标输入实现 / Mouse input implementation
 *
 * 实现 Mouse 类的所有成员函数，包括位置查询、按键状态查询、
 * 事件队列操作，以及由 Window 调用的输入回调函数。
 *
 * Implements all member functions of the Mouse class, including position
 * queries, button state queries, event queue operations, and input
 * callbacks called by Window.
 */

#include "Mouse.h"

namespace YingLong
{
	std::pair <int, int> Mouse::GetPos() const noexcept
	{
		return { x, y };
	}

	int Mouse::GetPosX() const noexcept
	{
		return x;
	}

	int Mouse::GetPosY() const noexcept
	{
		return y;
	}

	bool Mouse::LeftIsPressed() const noexcept
	{
		return leftIsPressed;
	}

	bool Mouse::RightIsPressed() const noexcept
	{
		return rightIsPressed;
	}

	Mouse::Event Mouse::Read() noexcept
	{
		// 队列非空则弹出队首事件，否则返回无效事件
		// Pop front event if queue not empty, otherwise return invalid event
		if (buffer.size() > 0u)
		{
			Mouse::Event e = buffer.front();
			buffer.pop();
			return e;
		}
		else
		{
			return Mouse::Event();
		}
	}

	void Mouse::Flush() noexcept
	{
		// 用空队列替换，清空事件队列
		// Replace with empty queue to flush event queue
		buffer = std::queue <Event>();
	}

	void Mouse::OnMouseMove(int NewX, int NewY) noexcept
	{
		// 更新位置 + 压入移动事件 + 修剪队列
		// Update position + push move event + trim queue
		x = NewX;
		y = NewY;

		buffer.push(Mouse::Event(Mouse::Event::Type::Move, *this));
		TrimBuffer();
	}

	void Mouse::TrimBuffer() noexcept
	{
		// 弹出最旧的事件直到队列大小符合限制
		// Pop oldest events until queue size meets the limit
		while (buffer.size() > BufferSize)
		{
			buffer.pop();
		}
	}
}
