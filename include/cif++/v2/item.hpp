/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 NKI/AVL, Netherlands Cancer Institute
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <cstring>

#include <iomanip>
#include <memory>
#include <optional>

namespace cif::v2
{

// --------------------------------------------------------------------
/// \brief item is a transient class that is used to pass data into rows

class item
{
  public:
	item() = default;

	item(std::string_view name, char value)
		: m_name(name)
		, m_value({ value })
	{
	}

	template <typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
	item(std::string_view name, const T &value, int precision)
		: m_name(name)
#if defined(__cpp_lib_format)
		, m_value(std::format(".{}f", value, precision))
#endif
	{
#if not defined(__cpp_lib_format)
		// TODO: implement
		m_value = std::to_string(value);
#endif
	}

	template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
	item(const std::string_view name, const T &value)
		: m_name(name)
		, m_value(std::to_string(value))
	{
	}

	item(const std::string_view name, const std::string_view value)
		: m_name(name)
		, m_value(value)
	{
	}

	item(const item &rhs) = default;

	item(item &&rhs) noexcept = default;

	item &operator=(const item &rhs) = default;

	item &operator=(item &&rhs) noexcept = default;

	const std::string &name() const { return m_name; }
	const std::string &value() const { return m_value; }

	void value(const std::string &v) { m_value = v; }

	/// \brief empty means either null or unknown
	bool empty() const { return m_value.empty(); }

	/// \brief returns true if the field contains '.'
	bool is_null() const { return m_value == "."; }

	/// \brief returns true if the field contains '?'
	bool is_unknown() const { return m_value == "?"; }

	size_t length() const { return m_value.length(); }
	const char *c_str() const { return m_value.c_str(); }

  private:
	std::string m_name;
	std::string m_value;
};



// --------------------------------------------------------------------
// Internal storage, strictly forward linked list with minimal space
// requirements.

struct item_value
{
	item_value(uint32_t column_ix, const char *value)
		: m_column_ix(column_ix)
	{
		strcpy(m_text, value);
	}

	item_value *m_next;
	uint32_t m_column_ix;
	char m_text[4];
};

static_assert(sizeof(item_value) == 2 * sizeof(void*), "Item value should be two pointers in size...");

// --------------------------------------------------------------------
// Transient object to access stored data

template<typename Row>
struct item_handle
{
	using row_type = Row;

  public:
	// conversion helper class
	template <typename T, typename = void>
	struct item_value_as;

	template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
	item_handle &operator=(const T &value)
	{
		this->operator=(std::to_string(value));
		return *this;
	}

	template <typename T>
	item_handle &operator=(const std::optional<T> &value)
	{
		if (value)
			this->operator=(*value);
		else
			this->operator=("?");
		return *this;
	}

	item_handle &operator=(const std::string &value)
	{
		m_row.assign(m_name, value, false);
		return *this;
	}

	template <typename... Ts>
	void os(const Ts &...v)
	{
		std::ostringstream ss;
		((ss << v), ...);
		this->operator=(ss.str());
	}

	void swap(item_handle &b);

	template <typename T = std::string>
	auto as() const
	{
		using value_type = std::remove_cv_t<std::remove_reference_t<T>>;
		return item_value_as<value_type>::convert(*this);
	}

	template <typename T>
	auto value_or(const T &dv)
	{
		return empty() ? dv : this->as<T>();
	}

	template <typename T>
	int compare(const T &value, bool icase) const
	{
		return item_value_as<T>::compare(*this, value, icase);
	}

	// empty means either null or unknown
	bool empty() const
	{
		const char *s = c_str();
		return *s == 0 or (s[1] == 0 and (*s == '.' or *s == '?'));
	}

	explicit operator bool() const { return not empty(); }

	// is_null means the field contains '.'
	bool is_null() const
	{
		const char *s = c_str();
		return *s == '.' and s[1] == 0;
	}

	// is_unknown means the field contains '?'
	bool is_unknown() const
	{
		const char *s = c_str();
		return *s == '?' and s[1] == 0;
	}

	const char *c_str() const
	{
		for (auto iv = m_row.m_head; iv != nullptr; iv = iv->m_next)
		{
			if (iv->m_column_ix == m_column)
				return iv->m_text;
		}

		return s_empty_result;
	}

	// bool operator!=(const std::string &s) const { return s != c_str(); }
	// bool operator==(const std::string &s) const { return s == c_str(); }

	item_handle(std::string_view name, size_t column, row_type &row)
		: m_name(name)
		, m_column(column)
		, m_row(row)
	{
	}

	item_handle(std::string_view name, size_t column, const row_type &row)
		: m_name(name)
		, m_column(column)
		, m_row(const_cast<row_type &>(row))
		// , mConst(true)
	{
	}

  private:

	std::string_view m_name;
	size_t m_column;
	row_type &m_row;
	// bool mConst = false;

	static constexpr const char *s_empty_result = "";
};

template <typename Row>
template <typename T>
struct item_handle<Row>::item_value_as<T, std::enable_if_t<std::is_floating_point_v<T>>>
{
	using value_type = std::remove_reference_t<std::remove_cv_t<T>>;

	static value_type convert(const item_handle &ref)
	{
		value_type result = {};
		if (not ref.empty())
			result = static_cast<value_type>(std::stod(ref.c_str()));
		return result;
	}

	static int compare(const item_handle &ref, double value, bool icase)
	{
		int result = 0;

		const char *s = ref.c_str();

		if (s == nullptr or *s == 0)
			result = 1;
		else
		{
			try
			{
				auto v = std::strtod(s, nullptr);
				if (v < value)
					result = -1;
				else if (v > value)
					result = 1;
			}
			catch (...)
			{
				// if (VERBOSE)
				// 	std::cerr << "conversion error in compare for '" << ref.c_str() << '\'' << std::endl;
				result = 1;
			}
		}

		return result;
	}
};

template <typename Row>
template <typename T>
struct item_handle<Row>::item_value_as<T, std::enable_if_t<std::is_integral_v<T> and std::is_unsigned_v<T> and not std::is_same_v<T, bool>>>
{
	static T convert(const item_handle &ref)
	{
		T result = {};
		if (not ref.empty())
			result = static_cast<T>(std::stoul(ref.c_str()));
		return result;
	}

	static int compare(const item_handle &ref, unsigned long value, bool icase)
	{
		int result = 0;

		const char *s = ref.c_str();

		if (s == nullptr or *s == 0)
			result = 1;
		else
		{
			try
			{
				auto v = std::strtoul(s, nullptr, 10);
				if (v < value)
					result = -1;
				else if (v > value)
					result = 1;
			}
			catch (...)
			{
				// if (VERBOSE)
				// 	std::cerr << "conversion error in compare for '" << ref.c_str() << '\'' << std::endl;
				result = 1;
			}
		}

		return result;
	}
};

template <typename Row>
template <typename T>
struct item_handle<Row>::item_value_as<T, std::enable_if_t<std::is_integral_v<T> and std::is_signed_v<T> and not std::is_same_v<T, bool>>>
{
	static T convert(const item_handle &ref)
	{
		T result = {};
		if (not ref.empty())
			result = static_cast<T>(std::stol(ref.c_str()));
		return result;
	}

	static int compare(const item_handle &ref, long value, bool icase)
	{
		int result = 0;

		const char *s = ref.c_str();

		if (s == nullptr or *s == 0)
			result = 1;
		else
		{
			try
			{
				auto v = std::strtol(s, nullptr, 10);
				if (v < value)
					result = -1;
				else if (v > value)
					result = 1;
			}
			catch (...)
			{
				// if (VERBOSE)
				// 	std::cerr << "conversion error in compare for '" << ref.c_str() << '\'' << std::endl;
				result = 1;
			}
		}

		return result;
	}
};

template <typename Row>
template <typename T>
struct item_handle<Row>::item_value_as<std::optional<T>>
{
	static std::optional<T> convert(const item_handle &ref)
	{
		std::optional<T> result;
		if (ref)
			result = ref.as<T>();
		return result;
	}

	static int compare(const item_handle &ref, std::optional<T> value, bool icase)
	{
		if (ref.empty() and not value)
			return 0;

		if (ref.empty())
			return -1;
		else if (not value)
			return 1;
		else
			return ref.compare(*value, icase);
	}
};

template <typename Row>
template <typename T>
struct item_handle<Row>::item_value_as<T, std::enable_if_t<std::is_same_v<T, bool>>>
{
	static bool convert(const item_handle &ref)
	{
		bool result = false;
		if (not ref.empty())
			result = iequals(ref.c_str(), "y");
		return result;
	}

	static int compare(const item_handle &ref, bool value, bool icase)
	{
		bool rv = convert(ref);
		return value && rv ? 0
							: (rv < value ? -1 : 1);
	}
};

template <typename Row>
template <size_t N>
struct item_handle<Row>::item_value_as<char[N]>
{
	static const char *convert(const item_handle &ref)
	{
		return ref.c_str();
	}

	static int compare(const item_handle &ref, const char (&value)[N], bool icase)
	{
		return icase ? cif::icompare(ref.c_str(), value) : std::strcmp(ref.c_str(), value);
	}
};

template <typename Row>
template <typename T>
struct item_handle<Row>::item_value_as<T, std::enable_if_t<std::is_same_v<T, const char *>>>
{
	static const char *convert(const item_handle &ref)
	{
		return ref.c_str();
	}

	static int compare(const item_handle &ref, const char *value, bool icase)
	{
		return icase ? cif::icompare(ref.c_str(), value) : std::strcmp(ref.c_str(), value);
	}
};

template <typename Row>
template <typename T>
struct item_handle<Row>::item_value_as<T, std::enable_if_t<std::is_same_v<T, std::string>>>
{
	static std::string convert(const item_handle &ref)
	{
		return ref.c_str();
	}

	static int compare(const item_handle &ref, const std::string &value, bool icase)
	{
		return icase ? cif::icompare(ref.c_str(), value) : std::strcmp(ref.c_str(), value.c_str());
	}
};

} // namespace cif::v2
