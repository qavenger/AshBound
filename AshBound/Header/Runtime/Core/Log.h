#pragma once

#include <string>

#include "Runtime/Core/LogLevels.h"

enum class LogColor
{
	Default,
	Yellow,
	Red,
	Green,
	Cyan,
	Magenta,
	White
};

enum class LogLevel
{
#define LOG_LEVEL_ENTRY(name, display, color) name,
#define DECLARE_LOG_LEVEL(name, display, color) LOG_LEVEL_ENTRY(name, display, color)
#include "Runtime/Core/LogLevels.def"
#undef DECLARE_LOG_LEVEL
#undef LOG_LEVEL_ENTRY
	Count
};

struct LogCategory
{
	const wchar_t* name = L"Log";
	LogLevel defaultLevel = LogLevel::Info;
	LogLevel runtimeLevel = LogLevel::Info;
	bool writeToFile = true;

	constexpr LogCategory(const wchar_t* inName, LogLevel inDefault)
		: name(inName)
		, defaultLevel(inDefault)
		, runtimeLevel(inDefault)
	{
	}

	bool IsEnabled(LogLevel level) const
	{
		return static_cast<size_t>(level) >= static_cast<size_t>(runtimeLevel);
	}

	void SetRuntimeLevel(LogLevel level)
	{
		runtimeLevel = level;
	}

	void SetWriteToFile(bool enable)
	{
		writeToFile = enable;
	}
};

class Log
{
public:
	static void Init();
	static void Shutdown();

	static void Write(const std::wstring& message);
	static void Write(const wchar_t* format, ...);

	static void Write(const LogCategory& category, LogLevel level, const std::wstring& message);
	static void Write(const LogCategory& category, LogLevel level, const wchar_t* format, ...);
};

#define DECLARE_LOG_CATEGORY_EXTERN(Name, DefaultLevel) extern LogCategory Name;
#define DEFINE_LOG_CATEGORY(Name, DefaultLevel) LogCategory Name(L#Name, LogLevel::DefaultLevel);

#define LOG(Category, Level, ...) \
	do { \
		if ((Category).IsEnabled(LogLevel::Level)) { \
			::Log::Write((Category), LogLevel::Level, __VA_ARGS__); \
		} \
	} while (0)

DECLARE_LOG_CATEGORY_EXTERN(LogTemp, Info)

#define LOG_LEVEL(Category, LevelValue, ...) \
	do { \
		if ((Category).IsEnabled(LevelValue)) { \
			::Log::Write((Category), (LevelValue), __VA_ARGS__); \
		} \
	} while (0)
