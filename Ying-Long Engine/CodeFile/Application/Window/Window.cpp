/**
 * @file Window.cpp
 * @brief 窗口管理实现 / Window management implementation
 *
 * 实现 Window 类的所有功能：窗口类注册、窗口创建、消息处理、
 * 键盘/鼠标输入转发、DX11/DX12 图形设备管理、以及自定义异常。
 *
 * Implements all Window class functionality: window class registration,
 * window creation, message handling, keyboard/mouse input forwarding,
 * DX11/DX12 graphics device management, and custom exceptions.
 */

#include "Window.h"
#include "../Application.h"
#include "../../Graphics/DX12/DX12.h"
#include "../../Debug/DX12Log.h"
#include <sstream>
#include <stdexcept>

// ImGui Win32 后端的窗口过程处理函数（外部链接）
// ImGui Win32 backend window procedure handler (external linkage)
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace YingLong
{
	// 静态成员定义 / Static member definitions
	Time Window::time;
	Timer Window::timer;

	/**
	 * @brief 窗口类构造函数：注册 WNDCLASSEX
	 *        WindowClass constructor: registers WNDCLASSEX
	 *
	 * 设置窗口类的所有属性（图标、光标、背景、窗口过程等），
	 * 并调用 RegisterClassEx 注册。
	 *
	 * Sets all window class attributes (icon, cursor, background, window
	 * procedure, etc.) and registers via RegisterClassEx.
	 *
	 * @param windowTitle 窗口标题（用作类名后缀）/ Window title (used as class name suffix)
	 * @param popup 是否为弹出窗口（决定使用哪个窗口过程）/
	 *        Whether it's a popup (determines which window proc to use)
	 */
	Window::WindowClass::WindowClass(const wchar_t* windowTitle, bool popup) : hInstance(GetModuleHandle(NULL))
	{
		// 构造唯一的窗口类名 / Construct unique window class name
		this->pClassName = windowTitle;
		this->pClassName += L" Window Class";

		WNDCLASSEX WindowClassEx = { 0 };
		WindowClassEx.cbSize = sizeof(WindowClassEx);
		WindowClassEx.style = CS_OWNDC;  // 每个窗口使用独立的 DC / Each window uses its own DC
		// 根据是否弹出窗口选择不同的窗口过程 / Choose window proc based on popup flag
		WindowClassEx.lpfnWndProc = popup ? PopupHandleMessageSetup : HandleMessageSetup;
		WindowClassEx.cbClsExtra = 0;
		WindowClassEx.cbWndExtra = 0;
		WindowClassEx.hInstance = hInstance;
		// 加载 128x128 大图标 / Load 128x128 large icon
		WindowClassEx.hIcon = (HICON)LoadImage(
			hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 128, 128, 0
		);
		WindowClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
		WindowClassEx.hbrBackground = NULL;
		WindowClassEx.lpszMenuName = NULL;
		WindowClassEx.lpszClassName = this->pClassName.c_str();
		// 加载 32x32 小图标 / Load 32x32 small icon
		WindowClassEx.hIconSm = (HICON)LoadImage(
			hInstance, MAKEINTRESOURCE(IDI_ICON2), IMAGE_ICON, 32, 32, 0
		);
		RegisterClassEx(&WindowClassEx);
	}

	/**
	 * @brief 窗口类析构函数：注销窗口类
	 *        WindowClass destructor: unregisters the window class
	 */
	Window::WindowClass::~WindowClass()
	{
		UnregisterClass(this->pClassName.c_str(), hInstance);
	}

	/**
	 * @brief Window 构造函数 / Window constructor
	 *
	 * 创建 Win32 窗口，居中显示。普通窗口会额外创建 Graphics 设备和相机。
	 * Creates a Win32 window, centered on screen. Normal windows additionally
	 * create a Graphics device and camera.
	 *
	 * @param width 窗口宽度 / Window width
	 * @param height 窗口高度 / Window height
	 * @param Name 窗口标题 / Window title
	 * @param popup 是否为弹出窗口 / Whether it's a popup window
	 */
	Window::Window(int width, int height, const wchar_t* Name, bool popup) : WindowClassObject(Name, popup)
	{
		// 初始化成员 / Initialize members
		this->IsDragging = false;
		this->pDX12Renderer = nullptr;
		this->bUseDX12 = false;

		this->Width = width;
		this->Height = height;

		// 根据 popup 标志选择窗口样式 / Choose window style based on popup flag
		UINT Style = 0;
		if (popup)
		{
			Style = WS_POPUPWINDOW;  // 弹出窗口样式（无标题栏）/ Popup window style (no title bar)
		}
		else
		{
			Style = WS_OVERLAPPEDWINDOW;  // 标准重叠窗口 / Standard overlapped window
		}

		// 计算窗口居中位置 / Calculate window center position
		int ScreenWidth = GetSystemMetrics(SM_CXSCREEN), ScreenHeight = GetSystemMetrics(SM_CYSCREEN);
		RECT WindowRect = { 0 };
		WindowRect.left = (ScreenWidth - width) / 2;
		WindowRect.right = width + WindowRect.left;
		WindowRect.top = (ScreenHeight - height) / 2;
		WindowRect.bottom = height + WindowRect.top;
		// 根据窗口样式调整客户区大小 / Adjust client area size based on window style
		if (FAILED(AdjustWindowRect(&WindowRect, Style, FALSE)))
		{
			throw WINDOW_LAST_EXCEPTION();
		}
		// 创建窗口，将 this 指针通过 lpParam 传给窗口过程
		// Create window, passing this pointer through lpParam to the window proc
		hWnd = CreateWindow(
			this->WindowClassObject.pClassName.c_str(), Name,
			Style,
			WindowRect.left, WindowRect.top,
			WindowRect.right - WindowRect.left,
			WindowRect.bottom - WindowRect.top,
			NULL, NULL, this->WindowClassObject.hInstance,
			this
		);
		if (hWnd == NULL)
		{
			throw WINDOW_LAST_EXCEPTION();
		}

		// 非弹出窗口创建图形设备和相机
		// Non-popup windows create graphics device and camera
		if (!popup)
		{
			pGraphics = std::make_unique <Graphics>(hWnd);
			this->camera = Camera();
		}
	}

	/**
	 * @brief 析构函数：调用 Destroy 销毁窗口
	 *        Destructor: calls Destroy to destroy the window
	 */
	Window::~Window()
	{
		this->Destroy();
	}

	/**
	 * @brief 显示窗口 / Show window
	 */
	void Window::Display()
	{
		::ShowWindow(hWnd, SW_SHOWDEFAULT);
	}

	/**
	 * @brief 销毁窗口 / Destroy window
	 *
	 * 销毁 Win32 窗口并将句柄置空。安全多次调用。
	 * Destroys the Win32 window and nulls the handle. Safe to call multiple times.
	 */
	void Window::Destroy()
	{
		if (hWnd == nullptr)
			return;

		DestroyWindow(hWnd);
		hWnd = nullptr;

		this->IsDragging = false;
	}

	/**
	 * @brief 获取 Graphics 引用 / Get Graphics reference
	 *
	 * 如果 pGraphics 为 null（例如弹出窗口），抛出运行时异常。
	 * Throws runtime_error if pGraphics is null (e.g. for popup windows).
	 *
	 * @return Graphics& 图形设备引用 / Graphics device reference
	 */
	Graphics& Window::graphics()
	{
		if (!pGraphics)
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN);
			std::cout << "Error: YingLong::Window::graphics() - pGraphics is null!";
			throw std::runtime_error("YingLong::Window::graphics() - pGraphics is null");
		}
		return *pGraphics;
	}

	/**
	 * @brief 处理窗口消息（静态工具函数）/ Process window messages (static utility)
	 *
	 * 从消息队列中取出所有消息并分发。收到 WM_QUIT 时返回退出码。
	 * Peeks and dispatches all messages from the queue. Returns exit code on WM_QUIT.
	 *
	 * @return std::optional<int> 退出码（WM_QUIT 时），否则为空
	 *         Exit code (on WM_QUIT), empty otherwise
	 */
	std::optional<int> Window::ProcessMessages() noexcept
	{
		MSG message;
		while (PeekMessage(&message, NULL, 0u, 0u, PM_REMOVE))
		{
			if (message.message == WM_QUIT)
			{
				return (int)message.wParam;
			}

			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		return { };
	}

	/**
	 * @brief 获取窗口句柄 / Get window handle
	 * @return HWND 窗口句柄 / Window handle
	 */
	HWND Window::GetWindowHandle() const
	{
		return this->hWnd;
	}

	// =========================================================================
	// 窗口过程设置：WM_NCCREATE 时存储 this 指针并重定向窗口过程
	// Window procedure setup: store this pointer and redirect window proc on WM_NCCREATE
	// =========================================================================

	/**
	 * @brief 普通窗口的初始窗口过程（设置阶段）
	 *        Initial window proc for normal windows (setup phase)
	 *
	 * 在 WM_NCCREATE 时从 CREATESTRUCT 中取出 this 指针，
	 * 存入 GWLP_USERDATA，然后将窗口过程重定向到 HandleMessageThunk。
	 *
	 * On WM_NCCREATE, extracts this pointer from CREATESTRUCT, stores it in
	 * GWLP_USERDATA, then redirects the window proc to HandleMessageThunk.
	 */
	LRESULT CALLBACK Window::HandleMessageSetup(HWND hWnd,
		UINT message, WPARAM wParam, LPARAM lParam) noexcept
	{
		if (message == WM_NCCREATE)
		{
			const CREATESTRUCTW* const pCreate
				= reinterpret_cast <CREATESTRUCTW*> (lParam);
			Window* const pWindow
				= static_cast <Window*> (pCreate->lpCreateParams);

			// 将 this 指针存入窗口用户数据区 / Store this pointer in window user data
			SetWindowLongPtr(hWnd, GWLP_USERDATA,
				reinterpret_cast <LONG_PTR> (pWindow));
			// 重定向窗口过程到 thunk 函数 / Redirect window proc to thunk function
			SetWindowLongPtr(hWnd, GWLP_WNDPROC,
				reinterpret_cast <LONG_PTR> (&Window::HandleMessageThunk));

			return pWindow->HandleMessage(hWnd, message, wParam, lParam);
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	/**
	 * @brief 普通窗口的窗口过程跳板函数
	 *        Window proc thunk function for normal windows
	 *
	 * 从 GWLP_USERDATA 中取出 Window 指针，调用成员窗口过程。
	 * Retrieves Window pointer from GWLP_USERDATA and calls member window proc.
	 */
	LRESULT WINAPI Window::HandleMessageThunk(HWND hWnd,
		UINT message, WPARAM wParam, LPARAM lParam) noexcept
	{
		Window* const pWindow = reinterpret_cast <Window*>
			(GetWindowLongPtr(hWnd, GWLP_USERDATA));

		return pWindow->HandleMessage(hWnd, message, wParam, lParam);
	}

	/**
	 * @brief 弹出窗口的初始窗口过程（设置阶段）
	 *        Initial window proc for popup windows (setup phase)
	 */
	LRESULT CALLBACK Window::PopupHandleMessageSetup(HWND hWnd,
		UINT message, WPARAM wParam, LPARAM lParam) noexcept
	{
		if (message == WM_NCCREATE)
		{
			const CREATESTRUCTW* const pCreate
				= reinterpret_cast <CREATESTRUCTW*> (lParam);
			Window* const pWindow
				= static_cast <Window*> (pCreate->lpCreateParams);

			SetWindowLongPtr(hWnd, GWLP_USERDATA,
				reinterpret_cast <LONG_PTR> (pWindow));
			SetWindowLongPtr(hWnd, GWLP_WNDPROC,
				reinterpret_cast <LONG_PTR> (&Window::PopupHandleMessageThunk));

			return pWindow->PopupHandleMessage(hWnd, message, wParam, lParam);
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	/**
	 * @brief 弹出窗口的窗口过程跳板函数
	 *        Window proc thunk function for popup windows
	 */
	LRESULT WINAPI Window::PopupHandleMessageThunk(HWND hWnd,
		UINT message, WPARAM wParam, LPARAM lParam) noexcept
	{
		Window* const pWindow = reinterpret_cast <Window*>
			(GetWindowLongPtr(hWnd, GWLP_USERDATA));

		return pWindow->PopupHandleMessage(hWnd, message, wParam, lParam);
	}

	// =========================================================================
	// 光标控制 / Cursor control
	// =========================================================================

	/// @brief 将光标限制在窗口客户区内 / Confine cursor to window client area
	void Window::ConfineCursor() noexcept
	{
		RECT rect;
		GetClientRect(hWnd, &rect);
		// 将客户区坐标转为屏幕坐标 / Convert client coords to screen coords
		MapWindowPoints(hWnd, nullptr, reinterpret_cast<POINT*>(&rect), 2);
		ClipCursor(&rect);
	}

	/// @brief 释放光标限制 / Free cursor confinement
	void Window::FreeCursor() noexcept
	{
		ClipCursor(nullptr);
	}

	/// @brief 隐藏光标（循环调用直到计数 < 0）/ Hide cursor (loop until count < 0)
	void Window::HideCursor() noexcept
	{
		while (::ShowCursor(FALSE) >= 0);
	}

	/// @brief 显示光标（循环调用直到计数 >= 0）/ Show cursor (loop until count >= 0)
	void Window::ShowCursor() noexcept
	{
		while (::ShowCursor(TRUE) < 0);
	}

	// =========================================================================
	// 普通窗口消息处理 / Normal window message handling
	// =========================================================================

	/**
	 * @brief 普通窗口的消息处理函数 / Message handler for normal window
	 *
	 * 处理键盘输入、窗口大小变化、激活/失焦等消息。
	 * 先交给 ImGui Win32 后端处理，未处理的再自行处理。
	 *
	 * Handles keyboard input, window resize, activate/focus, etc.
	 * First passes to ImGui Win32 backend, then handles unprocessed ones.
	 */
	LRESULT Window::HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept
	{
		// 先让 ImGui Win32 后端处理消息（如 ImGui 窗口拖拽、输入等）
		// Let ImGui Win32 backend handle messages first (e.g. ImGui window drag, input)
		if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		{
			return 0;
		}

		switch (message)
		{
		case WM_CLOSE:
			// 窗口关闭时发送退出消息 / Post quit message when window closes
			PostQuitMessage(0);
			return 0;
			break;

		case WM_KILLFOCUS:
			// 窗口失去焦点时清空按键状态，防止粘键 / Clear key state on focus loss to prevent sticky keys
			keyboard.ClearState();
			break;

		case WM_ACTIVATE:
			// 窗口激活/失焦时处理光标限制 / Handle cursor confinement on window activate/deactivate
			if (!CursorEnabled)
			{
				if (wParam & WA_ACTIVE)
				{
					ConfineCursor();
					HideCursor();
				}
				else
				{
					FreeCursor();
					ShowCursor();
				}
			}
			break;

		case WM_SYSKEYDOWN:
			// 系统按键按下（如 Alt+F4 等），同样交由 Keyboard 处理
			// System key down (e.g. Alt+F4), also handled by Keyboard
			if (!(lParam & 0x40000000) || keyboard.AutorepeatIsEnabled())
			{
				keyboard.OnKeyPressed(static_cast<unsigned char>(wParam));
			}
			break;

		case WM_SYSKEYUP:
			// 系统按键释放 / System key release
			keyboard.OnKeyReleased(static_cast<unsigned char>(wParam));
			break;

		case WM_CHAR:
			// 字符消息（用于文本输入）/ Character message (for text input)
			keyboard.OnChar(static_cast<unsigned char>(wParam));
			break;

		case WM_SIZE:
				{
					int newWidth = (int)LOWORD(lParam);
					int newHeight = (int)HIWORD(lParam);

					// 最小化或零尺寸时忽略 / Ignore when minimized or zero size
					if (wParam == SIZE_MINIMIZED || newWidth == 0 || newHeight == 0)
					{
						return 0;
					}

					// 更新存储的尺寸 / Update stored dimensions
					this->Width = newWidth;
					this->Height = newHeight;

					// 更新 DX11 图形设备分辨率 / Update DX11 graphics resolution
					if (this->pGraphics)
					{
						this->pGraphics->UpdateGUIGraphicsResolution(newWidth, newHeight);
					}

					// 更新 DX12 渲染器（调整交换链和资源）/ Update DX12 renderer (resize swap chain and resources)
					if (this->pDX12Renderer && this->bUseDX12)
					{
						this->pDX12Renderer->Resize(newWidth, newHeight);
					}
				}
				break;
		}

		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	/**
	 * @brief 弹出窗口的消息处理函数 / Message handler for popup window
	 *
	 * 处理启动画面窗口的拖拽移动、关闭、绘制启动图片等消息。
	 * Handles drag-move of splash window, close, painting startup image, etc.
	 */
	LRESULT Window::PopupHandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept
	{
		// 记录拖拽起始位置 / Record drag start position
		static int startX, startY;

		switch (message)
		{
		case WM_CLOSE:
			PostQuitMessage(0);
			return 0;
			break;

		case WM_LBUTTONDOWN:
		{
			// 左键按下：开始拖拽 / Left button down: start dragging
			this->IsDragging = true;
			startX = LOWORD(lParam);
			startY = HIWORD(lParam);

			return 0;
			break;
		}

		case WM_LBUTTONUP:
		{
			// 左键释放：停止拖拽 / Left button up: stop dragging
			this->IsDragging = false;

			return 0;
			break;
		}

		case WM_MOUSEMOVE:
		{
			UINT MouseX = (UINT)LOWORD(lParam);
			UINT MouseY = (UINT)HIWORD(lParam);

			// 拖拽中：根据偏移移动窗口 / While dragging: move window by offset
			if (this->IsDragging)
			{
				int currentX = LOWORD(lParam);
				int currentY = HIWORD(lParam);
				int offsetX = currentX - startX;
				int offsetY = currentY - startY;

				RECT newRect;
				GetWindowRect(hWnd, &newRect);
				int width = newRect.right - newRect.left;
				int height = newRect.bottom - newRect.top;
				SetWindowPos(hWnd, NULL, newRect.left + offsetX, newRect.top + offsetY, 1000, 800, 0);
			}

			return 0;
			break;
		}

		case WM_ACTIVATE:
			// 激活/失焦时的光标处理（同普通窗口）/ Cursor handling on activate/deactivate (same as normal window)
			if (!CursorEnabled)
			{
				if (wParam & WA_ACTIVE)
				{
					ConfineCursor();
					HideCursor();
				}
				else
				{
					FreeCursor();
					ShowCursor();
				}
			}
			break;

		case WM_PAINT:
		{
			HDC hdc, LocalDC;
			PAINTSTRUCT ps;

			hdc = BeginPaint(hWnd, &ps);

			// 创建兼容 DC 用于加载并显示启动图片 / Create compatible DC for loading and displaying splash image
			LocalDC = CreateCompatibleDC(hdc);
			HBITMAP Bitmap = (HBITMAP)LoadImage(NULL, L"Resources\\Icon\\Ying-Long.jpg", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
			if (Bitmap == NULL)
			{
				std::cerr << "Failed to load startup image!" << std::endl;
			}
			BITMAP qBitmap;
			GetObject((HGDIOBJ)Bitmap, sizeof(BITMAP), (LPVOID)&qBitmap);
			SelectObject(LocalDC, Bitmap);

			// 位图块传输到窗口 DC / Bit blit to window DC
			if (!BitBlt(hdc, 0, 0, qBitmap.bmWidth, qBitmap.bmHeight, LocalDC, 0, 0, SRCCOPY))
			{
				std::cerr << "Failed to bit blt!" << std::endl;
			}

			DeleteDC(LocalDC);
			DeleteObject(Bitmap);

			// 透明背景文字：显示"正在初始化" / Transparent background text: display "Initializing"
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, RGB(255, 255, 255));

			SetTextAlign(hdc, TA_CENTER);

			std::string m = "Ying-Long Engine - Initializing";
			TextOutA(hdc, 1000 / 2, 800 - 30, m.c_str(), (int)m.size());

			EndPaint(hWnd, &ps);

			return DefWindowProc(hWnd, message, wParam, lParam);
			break;
		}
		}

		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	// =========================================================================
	// 异常类实现 / Exception class implementations
	// =========================================================================

	// 窗口 HRESULT 异常构造 / Window HRESULT exception constructor
	Window::HRESULTException::HRESULTException(int line, const char* file,
		HRESULT HRESULTException) noexcept : Exception(line, file), 
		HRESULTWindowException(HRESULTException)
	{

	}

	// 异常描述（包含错误码和错误字符串）/ Exception description (includes error code and error string)
	const char* Window::HRESULTException::what() const noexcept
	{
		std::ostringstream WindowException;
		WindowException << GetType() << std::endl
			<< "Error Code: " << GetErrorCode() << std::endl
			<< "Error: " << GetErrorString() << std::endl
			<< GetOriginString();
		what_buffer = WindowException.str();
		return what_buffer.c_str();
	}

	// 异常类型名称 / Exception type name
	const char* Window::HRESULTException::GetType() const noexcept
	{
		return "Window Exception";
	}

	// HRESULT 错误码转人类可读字符串 / Translate HRESULT error code to human-readable string
	std::string Window::WinException::TranslateErrorCode
	(HRESULT hr) noexcept
	{
		char* pMessageBuffer = nullptr;
		DWORD nMsgLen = FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, hr,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<LPSTR>(&pMessageBuffer),
			0, nullptr
		);
		if (nMsgLen == 0)
		{
			return "Unknown error!";
		}
		std::string ErrorString = pMessageBuffer;
		LocalFree(pMessageBuffer);
		return ErrorString;
	}

	// 获取错误码 / Get error code
	HRESULT Window::HRESULTException::GetErrorCode() const noexcept
	{
		return HRESULTWindowException;
	}

	// 获取错误码字符串 / Get error code string
	std::string Window::HRESULTException::GetErrorString() const noexcept
	{
		return Window::WinException::TranslateErrorCode(HRESULTWindowException);
	}

	// 无图形设备异常类型名 / No graphics device exception type name
	const char* Window::NoGfxException::GetType() const noexcept
	{
		return "û��ͼ���쳣��";
	}

	// =========================================================================
	// DX12 渲染器管理 / DX12 renderer management
	// =========================================================================

	/**
	 * @brief 初始化 DX12 渲染器 / Initialize DX12 renderer
	 *
	 * 创建 DX12Renderer 实例并初始化。失败时回退到 DX11 模式。
	 * Creates a DX12Renderer instance and initializes it. Falls back to DX11 mode on failure.
	 */
	void Window::InitializeDX12()
	{
		DX12Log("[Window::InitializeDX12] === Starting DX12 Window Initialization ===\n");

		if (pDX12Renderer != nullptr)
		{
			DX12Log("[Window::InitializeDX12] DX12Renderer already exists, skipping\n");
			return;
		}

		DX12Log(("[Window::InitializeDX12] Window handle: 0x" + std::to_string(reinterpret_cast<uintptr_t>(hWnd))
			+ ", dimensions: " + std::to_string(Width) + " x " + std::to_string(Height) + "\n").c_str());

		try
			{
				pDX12Renderer = std::make_unique<DX12Renderer>();
				DX12Log("[Window::InitializeDX12] DX12Renderer instance created\n");

				pDX12Renderer->Initialize(hWnd, Width, Height);
				DX12LogSuccess("[Window::InitializeDX12] DX12Renderer initialized successfully\n");

				bUseDX12 = true;
				DX12Log("[Window::InitializeDX12] === DX12 Window Initialization Complete ===\n");
			}
			catch (const std::exception& e)
			{
				DX12LogError(("[Window::InitializeDX12] FAILED: " + std::string(e.what()) + "\n").c_str());
				std::cerr << "Failed to initialize DX12: " << e.what() << std::endl;

				pDX12Renderer.reset();
				bUseDX12 = false;

				DX12LogWarning("[Window::InitializeDX12] Falling back to DX11 mode\n");
			}
		}

	/**
	 * @brief 关闭 DX12 渲染器 / Shutdown DX12 renderer
	 */
		void Window::ShutdownDX12()
		{
			if (pDX12Renderer)
			{
				pDX12Renderer->Shutdown();
				pDX12Renderer.reset();
				bUseDX12 = false;
			}
		}

	/**
	 * @brief 获取 DX12 渲染器指针 / Get DX12 renderer pointer
	 * @return DX12Renderer* 渲染器指针（可能为 null）/ Renderer pointer (may be null)
	 */
		DX12Renderer* Window::GetDX12Renderer() const
		{
			return pDX12Renderer.get();
		}
}
