#pragma once

#include <string>
#include <vector>
#include <utility> 
#include <variant>
#include <cstdio>

static uint32_t RoundUp(uint32_t dwValue, uint32_t dwMult)
{
	if (dwMult == 0) { return dwValue; }

	int remainder = dwValue % dwMult;
	if (remainder == 0)
		return dwValue;

	return dwValue + dwMult - remainder;
}

template<typename TValue>
constexpr size_t FirstBitLow(TValue value)
{
    constexpr size_t nbits = sizeof(TValue) * 8;
    constexpr auto zero = TValue{};
    constexpr auto one = static_cast<TValue>(1);

    for (size_t i = 0; i < nbits; i++)
    {
        if ((value & (one << i)) != zero)
        {
            return i;
        }
    }

    return 0;
}


using FormatArg = std::variant<int, double, const char*, std::string>;

template <typename... Args>
static int call_sprintf_impl(char* buffer, const char* format, Args... args) 
{
    return std::sprintf(buffer, format, args...);
}

static int call_sprintf(char* buffer, const char* format, const std::vector<FormatArg>& args) 
{
    int res = 0;

    if (args.size() == 1) 
    {
        std::visit([&](auto&& arg) 
            {
                res = call_sprintf_impl(buffer, format, arg);
            }, args[0]);
    }
    else if (args.size() == 2) 
    {
        std::visit([&](auto&& a1) 
            {
            std::visit([&](auto&& a2) 
                {
                    res = call_sprintf_impl(buffer, format, a1, a2);
                }, args[1]);
            }, args[0]);
    }

    return res;
}

#include <kernel/memory.h>
enum class FormatType { INT, STRING, FLOAT, CHAR, OTHER };

static void AddArgument(std::vector<FormatArg>& vec, FormatType format, uint32_t esp)
{

    int32_t espRes = esp + 4 + (vec.size() + 2 * 4);
    auto test = *reinterpret_cast<uint32_t*>(g_memory.base + espRes);
    test = test;
    char* t = (char* )test;
    t = t;
}

static std::vector<FormatArg> parse_format_specifiers(const std::string& format, uint32_t esp)
{
    std::vector<FormatArg> specifiers;
    int curArgumentNumber = 0;

    for (size_t i = 0; i < format.size(); ) 
    {
        if (format[i] == '%') 
        {
            int count = 0;
            bool found = false;

            for (int j = 1; j < 5; j++)
            {
                switch (format[i + j])
                {
                case 'd': case 'i': case 'u': case 'x': case 'X': case 'o':
                    //specifiers.emplace_back(i, FormatType::INT);
                    AddArgument(specifiers, FormatType::INT, esp);
                    i += 2;
                    found = true;
                    break;
                case 'f': case 'F': case 'e': case 'E': case 'g': case 'G': case 'a': case 'A':
                   // specifiers.emplace_back(i, FormatType::FLOAT);
                    i += 2;
                    found = true;
                    AddArgument(specifiers, FormatType::FLOAT, esp);
                    break;
                case 'c':
                   // specifiers.emplace_back(i, FormatType::CHAR);
                    i += 2;
                    found = true;
                    AddArgument(specifiers, FormatType::CHAR, esp);
                    break;
                case 's':
                    //specifiers.emplace_back(i, FormatType::STRING);
                    i += 2;
                    found = true;
                    AddArgument(specifiers, FormatType::STRING, esp);
                    break;
                case '%': // escaped percent sign
                    i += 2;
                    found = true;
                    break;
                }

                if (found)
                {
                    curArgumentNumber++;
                    break;
                }
            }
        }
        else 
        {
            i++;
        }
    }

    return specifiers;
}