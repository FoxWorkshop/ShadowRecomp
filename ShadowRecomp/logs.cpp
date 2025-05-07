#include <kernel/function.h>

//todo handle linux support
Logger logs;

void Logger::SetTextColor(int color)
{
	SetConsoleTextAttribute(g_hStandardOutput, color);
}

void Logger::LogFile(const std::string_view str, std::filesystem::path fileName)
{
	auto path_string = fileName.string();
	SetTextColor(FOREGROUND_YELLOW | FOREGROUND_INTENSITY);
	fmt::println("{} '{}'...", str, path_string);
	SetTextColor(FOREGROUND_WHITE);
}

void Logger::Log(const std::string_view str, ELogType type, const char* func)
{

	switch (type)
	{
	case ELogType::Utility:
		SetTextColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		break;
	case ELogType::Warning:
		SetTextColor(FOREGROUND_YELLOW | FOREGROUND_INTENSITY);
		break;
	case ELogType::Error:
		SetTextColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
		break;
	default:
		SetTextColor(FOREGROUND_WHITE);
		break;
	}

	if (func)
	{
		fmt::println("[{}] {}", func, str);
	}
	else
	{
		fmt::println("{}", str);
	}

	SetTextColor(FOREGROUND_WHITE);
}

void Logger::SetHandleStandardOutput()
{
	g_hStandardOutput = GetStdHandle(STD_OUTPUT_HANDLE);
}

void PrintError(const char* a1)
{
	LOGN_ERROR(a1);
	printf("\n");
}

void DebugPrint(char* const Buffer, const char* const Format, va_list ArgList)
{
	vsprintf(Buffer, Format, ArgList);
	LOGN_ERROR(Buffer);
}


X86_FUNC_IMPL(__imp__sub_11AD0);
X86_FUNC(sub_11AD0)
{
	LOGN("PreMainLauncher");
	__imp__sub_11AD0(ctx, base);
}


X86_FUNC_IMPL(__imp__sub_2AD4E4);
X86_FUNC(sub_2AD4E4)
{
	LOGN("XLaunchNewImageA");
	__imp__sub_2AD4E4(ctx, base);
}

X86_FUNC_IMPL(__imp__sub_11430);
X86_FUNC(sub_11430)
{
	printf("\n");
	LOGN_WARNING("....RsRwInitialize....");
	__imp__sub_11430(ctx, base);
}

X86_FUNC_IMPL(__imp__sub_578B0);
X86_FUNC(sub_578B0)
{
	printf("\n");
	LOGN_UTIL("Successfully initialized RenderWare.");
	LOGN_WARNING("....Now Loading Game Files....");
	__imp__sub_578B0(ctx, base);
}

X86_FUNC_IMPL(__imp__sub_28AF30);
X86_FUNC(sub_28AF30)
{
	LOGN_UTIL("....ADVERTISE...");
	__imp__sub_28AF30(ctx, base);
}


void InitLogs()
{
	logs.SetHandleStandardOutput();
}

GUEST_FUNCTION_HOOK(sub_11210, PrintError);
GUEST_FUNCTION_HOOK(sub_37E7BB, DebugPrint);

