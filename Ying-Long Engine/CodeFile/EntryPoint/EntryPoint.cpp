/**
 * @file EntryPoint.cpp
 * @brief Ying-Long Engine 程序入口点 / Program entry point of Ying-Long Engine
 *
 * 本文件包含 main() 函数，是整个引擎的执行起点。
 * 负责创建 Application 单例、进入主消息循环，并捕获所有类型的异常。
 *
 * This file contains the main() function, the execution entry point of the
 * entire engine. It creates the Application singleton, enters the main
 * message loop, and catches all types of exceptions.
 */

#include "../Application/Application.h"
#include <iostream>

/**
 * @brief 程序主入口 / Program main entry point
 *
 * 执行流程 / Execution flow:
 *   1. 创建 YingLong::Application 单例，设置窗口标题
 *      Create YingLong::Application singleton with window title
 *   2. 调用 Application::Go() 进入主循环
 *      Call Application::Go() to enter main loop
 *   3. 捕获 YingLong::Exception / std::exception / 未知异常并弹窗提示
 *      Catch YingLong::Exception / std::exception / unknown exceptions and show message box
 *
 * @return int 程序退出码，正常返回 0，异常返回 -1
 *         Program exit code: 0 on success, -1 on exception
 */
int	main()
{
	try 
	{
		// 创建 Application 单例并启动主循环
		// Create Application singleton and start main loop
		YingLong::Application::Instance = std::make_unique<YingLong::Application>(L"应龙引擎");
		return YingLong::Application::Instance->Go();
	}
	catch (const YingLong::Exception& e)
	{
		// 引擎自定义异常：输出到 stderr 并弹出"中止/重试/忽略"对话框
		// Engine custom exception: output to stderr and show Abort/Retry/Ignore dialog
		std::cerr << e.what() << std::endl;
		MessageBoxA(nullptr, e.what(), e.GetType(), MB_ABORTRETRYIGNORE | MB_ICONWARNING);
	}
	catch (const std::exception& e)
	{
		// 标准库异常：如 std::bad_alloc、std::runtime_error 等
		// Standard library exceptions: e.g. std::bad_alloc, std::runtime_error
		std::cerr << e.what() << std::endl;
		MessageBoxA(nullptr, e.what(), "Standard Exception", MB_ABORTRETRYIGNORE | MB_ICONERROR);
	}
	catch (...)
	{
		// 未知异常（非继承自 std::exception 的异常）
		// Unknown exceptions (not derived from std::exception)
		std::cerr << "Unknown exception!" << std::endl;
		MessageBoxA(nullptr, "No details available!", "Unknown Exception", MB_ABORTRETRYIGNORE | MB_ICONERROR);
	}
	
	return -1;
}
