#pragma once
#include <iostream>
#include <string>
#include <ctime>
#include <chrono>

namespace YingLong
{
    class Time
    {
    public:
        std::string GetSystemTime() noexcept;
    };

	class Timer
	{
	public:
		Timer();
		float mark();
		float peek() const;

	private:
		std::chrono::steady_clock::time_point last;
	};
}
