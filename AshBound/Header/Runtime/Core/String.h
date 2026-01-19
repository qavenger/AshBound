#pragma once

#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

class String
{
public:
	String() = default;
	String(const wchar_t* value)
		: data(value ? value : L"")
	{
	}
	String(const std::wstring& value)
		: data(value)
	{
	}
	String(std::wstring&& value) noexcept
		: data(std::move(value))
	{
	}

	const std::wstring& Str() const { return data; }
	const wchar_t* c_str() const { return data.c_str(); }
	bool IsEmpty() const { return data.empty(); }
	size_t Length() const { return data.length(); }

	String& operator+=(const String& other)
	{
		data += other.data;
		return *this;
	}
	String& operator+=(const wchar_t* other)
	{
		if (other)
		{
			data += other;
		}
		return *this;
	}

	friend String operator+(const String& a, const String& b)
	{
		return String(a.data + b.data);
	}
	friend String operator+(const String& a, const wchar_t* b)
	{
		return String(a.data + (b ? b : L""));
	}
	friend String operator+(const wchar_t* a, const String& b)
	{
		return String((a ? a : L"") + b.data);
	}

	template<typename... Args>
	static String Format(const wchar_t* format, Args&&... args)
	{
		std::vector<std::wstring> argStrings;
		argStrings.reserve(sizeof...(Args));
		(argStrings.emplace_back(ToWString(std::forward<Args>(args))), ...);
		return String(FormatImpl(format, argStrings));
	}

private:
	template<typename T>
	static std::wstring ToWString(T&& value)
	{
		using Decayed = std::decay_t<T>;
		if constexpr (std::is_same_v<Decayed, String>)
		{
			return value.data;
		}
		else if constexpr (std::is_same_v<Decayed, std::wstring>)
		{
			return value;
		}
		else if constexpr (std::is_same_v<Decayed, const wchar_t*> || std::is_same_v<Decayed, wchar_t*>)
		{
			return value ? value : L"";
		}
		else if constexpr (std::is_same_v<Decayed, bool>)
		{
			return value ? L"true" : L"false";
		}
		else if constexpr (std::is_arithmetic_v<Decayed>)
		{
			std::wostringstream oss;
			oss << value;
			return oss.str();
		}
		else
		{
			std::wostringstream oss;
			oss << value;
			return oss.str();
		}
	}

	static std::wstring FormatImpl(const wchar_t* format, const std::vector<std::wstring>& args)
	{
		if (!format)
		{
			return L"";
		}

		std::wstring result;
		for (size_t i = 0; format[i] != L'\0';)
		{
			const wchar_t ch = format[i];
			if (ch == L'{' && format[i + 1] == L'{')
			{
				result.push_back(L'{');
				i += 2;
				continue;
			}
			if (ch == L'}' && format[i + 1] == L'}')
			{
				result.push_back(L'}');
				i += 2;
				continue;
			}
			if (ch == L'{')
			{
				size_t j = i + 1;
				size_t index = 0;
				bool hasDigits = false;
				while (format[j] >= L'0' && format[j] <= L'9')
				{
					hasDigits = true;
					index = index * 10 + static_cast<size_t>(format[j] - L'0');
					++j;
				}
				if (hasDigits && format[j] == L'}')
				{
					if (index < args.size())
					{
						result += args[index];
					}
					else
					{
						result.append(format + i, (j - i) + 1);
					}
					i = j + 1;
					continue;
				}
			}

			result.push_back(ch);
			++i;
		}
		return result;
	}

	std::wstring data;
};

template<typename... Args>
inline String Text(const wchar_t* format, Args&&... args)
{
	return String::Format(format, std::forward<Args>(args)...);
}
