#define _CRT_SECURE_NO_WARNINGS
#include "Time.h"

namespace YingLong
{
	std::string Time::GetSystemTime() noexcept
	{
        // ��ȡ��ǰʱ���
        std::time_t CurrentTime = std::time(nullptr);

        // ת��Ϊ����ʱ��
        std::tm* LocalTime = std::localtime(&CurrentTime);
        if (LocalTime == nullptr)
        {
            return "��ȡʱ��ʧ�ܣ�";
        }

        // ��ʽ��ʱ���ַ���
        char TimeStr[20];  // �㹻�洢"YYYY-MM-DD HH:MM:SS"��ʽ
        std::strftime(TimeStr, sizeof(TimeStr), "%Y-%m-%d %H:%M:%S", LocalTime);

        return std::string(TimeStr);
	}

	Timer::Timer()
	{
		last = std::chrono::steady_clock::now();
	}

	float Timer::mark()
	{
		const auto old = last;
		last = std::chrono::steady_clock::now();
		const std::chrono::duration<float> FrameTime = last - old;
		return FrameTime.count();
	}

	float Timer::peek() const
	{
		return std::chrono::duration<float>
			(std::chrono::steady_clock::now() - last).count();
	}
}
