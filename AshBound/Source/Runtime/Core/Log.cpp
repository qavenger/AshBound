#include "Runtime/Core/Log.h"

#include <Windows.h>
#include <chrono>
#include <ctime>
#include <cstdarg>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace
{
	struct LogLevelDesc
	{
		const wchar_t* name;
		LogColor color;
	};

	HANDLE g_consoleHandle = nullptr;
	WORD g_defaultAttributes = 0;
	bool g_initialized = false;
	std::wofstream g_logFile;

	const LogLevelDesc g_logLevelDescs[] =
	{
#define LOG_LEVEL_ENTRY(name, display, color) { L#display, LogColor::color },
#define DECLARE_LOG_LEVEL(name, display, color) LOG_LEVEL_ENTRY(name, display, color)
#include "Runtime/Core/LogLevels.def"
#undef DECLARE_LOG_LEVEL
#undef LOG_LEVEL_ENTRY
	};

	static_assert(static_cast<size_t>(LogLevel::Count) ==
		sizeof(g_logLevelDescs) / sizeof(g_logLevelDescs[0]));

	std::wstring BuildLogFilePath()
	{
		const auto now = std::chrono::system_clock::now();
		const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
		std::tm localTime = {};
		localtime_s(&localTime, &nowTime);

		wchar_t timeBuffer[32] = {};
		wcsftime(timeBuffer, sizeof(timeBuffer) / sizeof(wchar_t), L"%Y%m%d_%H%M%S", &localTime);

		std::filesystem::path logDir = std::filesystem::path(L"Saved") / L"Logs";
		std::filesystem::create_directories(logDir);

		std::filesystem::path logFile = logDir / (std::wstring(L"AshBound_") + timeBuffer + L".log");
		return logFile.wstring();
	}

	void EnsureInitialized()
	{
		if (g_initialized)
		{
			return;
		}

		g_consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		if (!g_consoleHandle || g_consoleHandle == INVALID_HANDLE_VALUE)
		{
			return;
		}

		CONSOLE_SCREEN_BUFFER_INFO info = {};
		if (GetConsoleScreenBufferInfo(g_consoleHandle, &info))
		{
			g_defaultAttributes = info.wAttributes;
		}

		const std::wstring logFilePath = BuildLogFilePath();
		g_logFile.open(logFilePath, std::ios::out | std::ios::app);

		g_initialized = true;
	}

	size_t ToLevelIndex(LogLevel level)
	{
		const size_t index = static_cast<size_t>(level);
		const size_t maxIndex = static_cast<size_t>(LogLevel::Count);
		return index < maxIndex ? index : 0;
	}

	const LogLevelDesc& GetLevelDesc(LogLevel level)
	{
		return g_logLevelDescs[ToLevelIndex(level)];
	}

	WORD GetAttributesForColor(LogColor color)
	{
		switch (color)
		{
		case LogColor::Yellow:
			return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
		case LogColor::Red:
			return FOREGROUND_RED | FOREGROUND_INTENSITY;
		case LogColor::Green:
			return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
		case LogColor::Cyan:
			return FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
		case LogColor::Magenta:
			return FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
		case LogColor::White:
			return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
		case LogColor::Default:
		default:
			return g_defaultAttributes ? g_defaultAttributes : (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		}
	}

	std::wstring BuildTimestamp()
	{
		const auto now = std::chrono::system_clock::now();
		const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
		std::tm localTime = {};
		localtime_s(&localTime, &nowTime);

		wchar_t timeBuffer[32] = {};
		wcsftime(timeBuffer, sizeof(timeBuffer) / sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", &localTime);
		return std::wstring(timeBuffer);
	}

	void WriteLog(const LogCategory& category, LogLevel level, const std::wstring& message)
	{
		EnsureInitialized();
		const LogLevelDesc& desc = GetLevelDesc(level);
		const std::wstring timestamp = BuildTimestamp();
		std::wstring fullMessage = L"[";
		fullMessage += timestamp;
		fullMessage += L"] [";
		fullMessage += desc.name;
		fullMessage += L"] [";
		fullMessage += category.name ? category.name : L"Log";
		fullMessage += L"] ";
		fullMessage += message;

		if (g_consoleHandle && g_consoleHandle != INVALID_HANDLE_VALUE)
		{
			SetConsoleTextAttribute(g_consoleHandle, GetAttributesForColor(desc.color));
		}

		std::wcout << fullMessage << std::endl;
		if (category.writeToFile && g_logFile.is_open())
		{
			g_logFile << fullMessage << std::endl;
		}

		if (g_consoleHandle && g_consoleHandle != INVALID_HANDLE_VALUE)
		{
			SetConsoleTextAttribute(g_consoleHandle, g_defaultAttributes);
		}
	}
}

DEFINE_LOG_CATEGORY(LogTemp, Info)

void Log::Init()
{
	EnsureInitialized();
}

void Log::Shutdown()
{
	if (g_consoleHandle && g_consoleHandle != INVALID_HANDLE_VALUE && g_defaultAttributes != 0)
	{
		SetConsoleTextAttribute(g_consoleHandle, g_defaultAttributes);
	}

	if (g_logFile.is_open())
	{
		g_logFile.flush();
		g_logFile.close();
	}
}

void Log::Write(const std::wstring& message)
{
	if (!LogTemp.IsEnabled(LogLevel::Info))
	{
		return;
	}
	WriteLog(LogTemp, LogLevel::Info, message);
}

void Log::Write(const wchar_t* format, ...)
{
	if (!LogTemp.IsEnabled(LogLevel::Info))
	{
		return;
	}
	if (!format)
	{
		return;
	}
	wchar_t buffer[1024] = {};
	va_list args;
	va_start(args, format);
	vswprintf_s(buffer, format, args);
	va_end(args);
	WriteLog(LogTemp, LogLevel::Info, buffer);
}

void Log::Write(const LogCategory& category, LogLevel level, const std::wstring& message)
{
	if (!category.IsEnabled(level))
	{
		return;
	}
	WriteLog(category, level, message);
}

void Log::Write(const LogCategory& category, LogLevel level, const wchar_t* format, ...)
{
	if (!category.IsEnabled(level))
	{
		return;
	}
	if (!format)
	{
		return;
	}
	wchar_t buffer[1024] = {};
	va_list args;
	va_start(args, format);
	vswprintf_s(buffer, format, args);
	va_end(args);
	Write(category, level, std::wstring(buffer));
}
