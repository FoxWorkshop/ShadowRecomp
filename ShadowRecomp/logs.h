#pragma once

#include <source_location>

#define FOREGROUND_WHITE  (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define FOREGROUND_YELLOW (FOREGROUND_RED | FOREGROUND_GREEN)

enum class ELogType
{
	None,
	Utility,
	Warning,
	Error
};

class Logger
{
private:
	HANDLE g_hStandardOutput;
public:

	void SetTextColor(int color);
	void Log(const std::string_view str, ELogType type = ELogType::None, const char* func = nullptr);
	void SetHandleStandardOutput();
	void LogFile(const std::string_view str, std::filesystem::path fileName);
};

extern Logger logs;

#define LOG_IMPL(type, func, str)       logs.Log(str, ELogType::type, func)
#define LOGF_IMPL(type, func, str, ...) logs.Log(fmt::format(str, __VA_ARGS__), ELogType::type, func)

#define LOG(str)               LOG_IMPL(None, __func__, str)
#define LOG_WARNING(str)       LOG_IMPL(Warning, __func__, str)
#define LOG_ERROR(str)         LOG_IMPL(Error, __func__, str)
#define LOG_UTIL(str)       LOG_IMPL(Utility, __func__, str)

#define LOGN(str)               LOG_IMPL(None, "*", str)
#define LOGN_WARNING(str)       LOG_IMPL(Warning, "*", str)
#define LOGN_ERROR(str)         LOG_IMPL(Error, "*", str)
#define LOGN_UTIL(str)       LOG_IMPL(Utility, "*", str)

void InitLogs();