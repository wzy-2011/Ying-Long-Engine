/**
 * @file Keyboard.cpp
 * @brief 键盘输入实现 / Keyboard input implementation
 *
 * 实现 Keyboard 类的所有成员函数，包括状态查询、事件队列操作、
 * 以及由 Window 调用的输入回调函数。
 *
 * Implements all member functions of the Keyboard class, including state
 * queries, event queue operations, and input callbacks called by Window.
 */

#include "Keyboard.h"

namespace YingLong
{
	bool Keyboard::KeyIsPressed(unsigned char keycode) const noexcept
	{
		// 直接从 bitset 读取对应位的状态
		// Read the corresponding bit state directly from the bitset
		return keystates[keycode];
	}

	std::optional<Keyboard::Event> Keyboard::ReadKey() noexcept
	{
		// 如果队列非空，弹出队首事件并返回
		// If queue is not empty, pop front event and return
		if (keybuffer.size() > 0u)
		{
			Keyboard::Event e = keybuffer.front();
			keybuffer.pop();
			return e;
		}
		return {};
	}

	bool Keyboard::KeyIsEmpty() const noexcept
	{
		return keybuffer.empty();
	}

	std::optional<char> Keyboard::ReadChar() noexcept
	{
		// 从字符队列弹出队首字符
		// Pop front char from char queue
		if (charbuffer.size() > 0u)
		{
			unsigned char charcode = charbuffer.front();
			charbuffer.pop();
			return charcode;
		}
		return {};
	}

	bool Keyboard::CharIsEmpty() const noexcept
	{
		return charbuffer.empty();
	}

	void Keyboard::FlushKey() noexcept
	{
		// 用空队列替换，清空按键事件队列
		// Replace with empty queue to flush key event queue
		keybuffer = std::queue<Event>();
	}

	void Keyboard::FlushChar() noexcept
	{
		// 用空队列替换，清空字符事件队列
		// Replace with empty queue to flush char event queue
		charbuffer = std::queue<char>();
	}

	void Keyboard::Flush() noexcept
	{
		// 同时清空按键和字符队列
		// Flush both key and char queues
		FlushKey();
		FlushChar();
	}

	void Keyboard::EnableAutorepeat() noexcept
	{
		autorepeatEnabled = true;
	}

	void Keyboard::DisableAutorepeat() noexcept
	{
		autorepeatEnabled = false;
	}

	bool Keyboard::AutorepeatIsEnabled() const noexcept
	{
		return autorepeatEnabled;
	}

	void Keyboard::OnKeyPressed(unsigned char keycode) noexcept
	{
		// 更新按键状态 + 压入按下事件 + 修剪队列
		// Update key state + push press event + trim queue
		keystates[keycode] = true;
		keybuffer.push(Keyboard::Event(Keyboard::Event::Type::Press, keycode));
		TrimBuffer(keybuffer);
	}

	void Keyboard::OnKeyReleased(unsigned char keycode) noexcept
	{
		// 更新按键状态 + 压入释放事件 + 修剪队列
		// Update key state + push release event + trim queue
		keystates[keycode] = false;
		keybuffer.push(Keyboard::Event(Keyboard::Event::Type::Release, keycode));
		TrimBuffer(keybuffer);
	}

	void Keyboard::OnChar(char character) noexcept
	{
		// 压入字符事件 + 修剪队列
		// Push char event + trim queue
		charbuffer.push(character);
		TrimBuffer(charbuffer);
	}

	void Keyboard::ClearState() noexcept
	{
		// 窗口失去焦点时重置所有按键状态，防止"粘键"
		// Reset all key states when window loses focus to prevent "sticky keys"
		keystates.reset();
	}

	// 模板函数：修剪队列到最大容量
	// Template function: trim queue to max capacity
	template<typename T>
	void Keyboard::TrimBuffer(std::queue<T>& buffer) noexcept
	{
		// 弹出最旧的元素直到队列大小符合限制
		// Pop oldest elements until queue size meets the limit
		while (buffer.size() > bufferSize)
		{
			buffer.pop();
		}
	}
}
