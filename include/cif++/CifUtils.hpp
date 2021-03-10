/*-
 * SPDX-License-Identifier: BSD-2-Clause
 * 
 * Copyright (c) 2020 NKI/AVL, Netherlands Cancer Institute
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

#include <vector>
#include <set>
#include <cassert>
#include <memory>
#include <list>
#include <iostream>
#include <filesystem>

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

#if _MSC_VER
constexpr inline bool isatty(int) { return false; }
#endif

namespace cif
{

// the git 'build' number
std::string get_version_nr();
// std::string get_version_date();

// --------------------------------------------------------------------

// some basic utilities: Since we're using ASCII input only, we define for optimisation
// our own case conversion routines.

bool iequals(const std::string& a, const std::string& b);
int icompare(const std::string& a, const std::string& b);

bool iequals(const char* a, const char* b);
int icompare(const char* a, const char* b);

void toLower(std::string& s);
std::string toLowerCopy(const std::string& s);

// To make life easier, we also define iless and iset using iequals

struct iless
{
	bool operator()(const std::string& a, const std::string& b) const
	{
		return icompare(a, b) < 0;
	}
};

typedef std::set<std::string, iless>	iset;

// --------------------------------------------------------------------
// This really makes a difference, having our own tolower routines

extern const uint8_t kCharToLowerMap[256];

inline char tolower(char ch)
{
	return static_cast<char>(kCharToLowerMap[static_cast<uint8_t>(ch)]);
}

// --------------------------------------------------------------------

std::tuple<std::string,std::string> splitTagName(const std::string& tag);

// --------------------------------------------------------------------
//	custom wordwrapping routine

std::vector<std::string> wordWrap(const std::string& text, unsigned int width);

// --------------------------------------------------------------------
//	Code helping with terminal i/o

uint32_t get_terminal_width();

// --------------------------------------------------------------------
//	Path of the current executable

std::string get_executable_path();

// --------------------------------------------------------------------
//	some manipulators to write coloured text to terminals

enum StringColour {
	scBLACK = 0, scRED, scGREEN, scYELLOW, scBLUE, scMAGENTA, scCYAN, scWHITE, scNONE = 9 };

template<typename String, typename CharT>
struct ColouredString
{
	static_assert(std::is_reference<String>::value or std::is_pointer<String>::value, "String type must be pointer or reference");
	
	ColouredString(String s, StringColour fore, StringColour back, bool bold = true)
		: m_s(s), m_fore(fore), m_back(back), m_bold(bold) {}
	
	ColouredString& operator=(const ColouredString&) = delete;
	
	String m_s;
	StringColour m_fore, m_back;
	bool m_bold;
};

template<typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const ColouredString<const CharT*,CharT>& s)
{
	if (isatty(STDOUT_FILENO))
	{
		std::basic_ostringstream<CharT, Traits> ostr;
		ostr << "\033[" << (30 + s.m_fore) << ';' << (s.m_bold ? "1" : "22") << ';' << (40 + s.m_back) << 'm'
			 << s.m_s
			 << "\033[0m";
		
		return os << ostr.str();
	}
	else
		return os << s.m_s;
}

template<typename CharT, typename Traits, typename String>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const ColouredString<String,CharT>& s)
{
	if (isatty(STDOUT_FILENO))
	{
		std::basic_ostringstream<CharT, Traits> ostr;
		ostr << "\033[" << (30 + s.m_fore) << ';' << (s.m_bold ? "1" : "22") << ';' << (40 + s.m_back) << 'm'
			 << s.m_s
			 << "\033[0m";
		
		return os << ostr.str();
	}
	else
		return os << s.m_s;
}

template<typename CharT>
inline auto coloured(const CharT* s, StringColour fore = scWHITE, StringColour back = scRED, bool bold = true)
{
	return ColouredString<const CharT*, CharT>(s, fore, back, bold);
}

template<typename CharT, typename Traits, typename Alloc>
inline auto coloured(const std::basic_string<CharT, Traits, Alloc>& s, StringColour fore = scWHITE, StringColour back = scRED, bool bold = true)
{
	return ColouredString<const std::basic_string<CharT, Traits, Alloc>, CharT>(s, fore, back, bold);
}

template<typename CharT, typename Traits, typename Alloc>
inline auto coloured(std::basic_string<CharT, Traits, Alloc>& s, StringColour fore = scWHITE, StringColour back = scRED, bool bold = true)
{
	return ColouredString<std::basic_string<CharT, Traits, Alloc>, CharT>(s, fore, back, bold);
}

// --------------------------------------------------------------------
//	A progress bar

class Progress
{
  public:
				Progress(int64_t inMax, const std::string& inAction);
	virtual		~Progress();
	
	void		consumed(int64_t inConsumed);	// consumed is relative
	void		progress(int64_t inProgress);	// progress is absolute

	void		message(const std::string& inMessage);

  private:
				Progress(const Progress&) = delete;
	Progress& operator=(const Progress&) = delete;

	struct ProgressImpl*	mImpl;
};

// --------------------------------------------------------------------
// Resources

std::unique_ptr<std::istream> loadResource(std::filesystem::path name);

}


