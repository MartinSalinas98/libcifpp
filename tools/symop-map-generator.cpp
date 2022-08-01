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

#include <cassert>

#include <array>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <regex>
#include <map>
#include <filesystem>

#include <boost/program_options.hpp>

#include <cstdlib>

namespace fs = std::filesystem;
namespace po = boost::program_options;

std::regex kNameRx(R"(^(\d+) +(\d+) +(\d+) +(\S+) +(\S+) +(\S+) +'([^']+)'( +'([^']+)')?(?: +!.+)?$)");

class SymopParser
{
  public:
	SymopParser() {}

	std::array<int,15> parse(const std::string& s)
	{
		m_p = s.begin();
		m_e = s.end();
		m_lookahead = next_token();

		parsepart(0);
		match((Token)',');
		parsepart(1);
		match((Token)',');
		parsepart(2);

		if (m_lookahead != 0 or m_p != m_e)
			throw std::runtime_error("symmetry expression contains more data than expected");

		return {
			m_rot[0][0], m_rot[0][1], m_rot[0][2], 
			m_rot[1][0], m_rot[1][1], m_rot[1][2], 
			m_rot[2][0], m_rot[2][1], m_rot[2][2], 
			m_trn[0][0], m_trn[0][1],
			m_trn[1][0], m_trn[1][1],
			m_trn[2][0], m_trn[2][1]
		};
	}

  private:

	enum Token : int { Eof = 0, Number = 256, XYZ };

	std::string to_string(Token t)
	{
		switch (t)
		{
			case Eof:		return "end of expression";
			case Number:	return "number";
			case XYZ:		return "'x', 'y' or 'z'";
			default:
				if (isprint(t))
					return std::string({'\'', static_cast<char>(t), '\''});
				return "invalid character " + std::to_string(static_cast<int>(t));
		}
	}
	
	Token next_token()
	{
		Token result = Eof;
		while (m_p != m_e)
		{
			char ch = *m_p++;
			if (ch == ' ')
				continue;

			switch (ch)
			{
				case 'x':
				case 'X':
					result = XYZ;
					m_nr = 0;
					break;
				
				case 'y':
				case 'Y':
					result = XYZ;
					m_nr = 1;
					break;
				
				case 'z':
				case 'Z':
					result = XYZ;
					m_nr = 2;
					break;
				
				default:
					if (isdigit(ch))
					{
						m_nr = ch - '0';
						result = Number;
					}
					else
						result = (Token)ch;
					break;
			}
			break;
		}

		return result;
	}

	void match(Token token)
	{
		if (m_lookahead != token)
			throw std::runtime_error("Unexpected character " + to_string(m_lookahead) + " expected " + to_string(token));
		
		m_lookahead = next_token();
	}

	void parsepart(int row)
	{
		do
		{
			int sign = m_lookahead == '-' ? -1 : 1;
			if (m_lookahead == '-' or m_lookahead == '+')
				match(m_lookahead);

			if (m_lookahead == Number)
			{
				m_trn[row][0] = sign * m_nr;
				match(Number);

				match((Token)'/');

				m_trn[row][1] = m_nr;
				match(Number);
			}
			else
			{
				m_rot[row][m_nr] = sign;
				match(XYZ);
			}
		}
		while (m_lookahead == '+' or m_lookahead == '-');
	}

	Token m_lookahead;
	int m_nr;

	std::string m_s;
	std::string::const_iterator m_p, m_e;

	int m_rot[3][3] = {};
	int m_trn[3][2] = {};
};

std::array<int,15> move_symop(std::array<int,15> symop, const std::array<int,15>& cenop)
{
	for (int i = 9; i < 15; i += 2)
	{
		if (cenop[i] == 0)
			continue;
		
		assert(cenop[i + 1] != 0);

		if (symop[i] == 0)
		{
			assert(symop[i + 1] == 0);
			symop[i] = cenop[i];
			symop[i + 1] = cenop[i + 1];
			continue;
		}

		if (symop[i + 1] == cenop[i + 1])
			symop[i] += cenop[i];
		else
		{
			int d = symop[i + 1] * cenop[i + 1];
			int n = symop[i] * cenop[i + 1] + symop[i + 1] * cenop[i];

			symop[i] = n;
			symop[i + 1] = d;
		}

		for (int j = 5; j > 1; --j)
			if (symop[i] % j == 0 and symop[i + 1] % j == 0)
			{
				symop[i] /= j;
				symop[i + 1] /= j;
			}
		
		symop[i] = (symop[i] + symop[i + 1]) % symop[i + 1];

		if (symop[i] == 0)
			symop[i + 1] = 0;
	}

	return symop;
}

int main(int argc, char* const argv[])
{
	using namespace std::literals;

	fs::path tmpFile;

	try
	{
		po::options_description visible_options("symop-map-generator symlib-file output-file");
		visible_options.add_options()
			( "help,h",							"Display help message" )
			( "verbose,v",						"Verbose output") ;

		po::options_description hidden_options("hidden options");
		hidden_options.add_options()
			( "input,i", po::value<std::string>(), "Input file")
			( "output,o", po::value<std::string>(), "Output file");

		po::options_description cmdline_options;
		cmdline_options.add(visible_options).add(hidden_options);

		po::positional_options_description p;
		p.add("input", 1);
		p.add("output", 1);

		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
		po::notify(vm);
		
		if (vm.count("input") == 0 or vm.count("output") == 0 or vm.count("help"))
		{
			std::cerr << visible_options << std::endl;
			exit(vm.count("help") == 0);
		}

		fs::path input(vm["input"].as<std::string>());
		fs::path output(vm["output"].as<std::string>());
		
		tmpFile = output.parent_path() / (output.filename().string() + ".tmp");

		std::ofstream out(tmpFile);
		if (not out.is_open())
			throw std::runtime_error("Failed to open output file");

		// --------------------------------------------------------------------

		// store symop data here
		std::vector<std::tuple<int,int,std::array<int,15>>> data;

		// -----------------------------------------------------------------------
		
		struct SymInfoBlock
		{
			int nr;
			std::string xHM;
			std::string Hall;
			std::string old[2];
		};

		std::map<int,SymInfoBlock> symInfo;
		int symopnr, mysymnr = 10000;

		std::ifstream file(input);
		if (not file.is_open())
			throw std::runtime_error("Could not open syminfo.lib file");

		enum class State { skip, spacegroup } state = State::skip;

		std::string line;

		const std::regex rx(R"(^symbol +(Hall|xHM|old) +'(.+?)'(?: +'(.+?)')?$)"),
			rx2(R"(symbol ccp4 (\d+))");;

		SymInfoBlock cur = {};

		std::vector<std::array<int,15>> symops, cenops;

		while (getline(file, line))
		{
			switch (state)
			{
				case State::skip:
					if (line == "begin_spacegroup")
					{
						state = State::spacegroup;
						symopnr = 1;
						++mysymnr;
						cur = { mysymnr };
					}
					break;
				
				case State::spacegroup:
				{
					std::smatch m;
					if (std::regex_match(line, m, rx))
					{
						if (m[1] == "old")
						{
							cur.old[0] = m[2];
							if (m[3].matched)
								cur.old[1] = m[3];
						}
						else if (m[1] == "xHM")
							cur.xHM = m[2];
						else if (m[1] == "Hall")
							cur.Hall = m[2];
					}
					else if (regex_match(line, m, rx2))
					{
						int nr = stoi(m[1]);
						if (nr != 0)
							cur.nr = nr;
					}
					else if (line.compare(0, 6, "symop ") == 0)
					{
						SymopParser p;
						symops.emplace_back(p.parse(line.substr(6)));
					}
					else if (line.compare(0, 6, "cenop ") == 0)
					{
						SymopParser p;
						cenops.emplace_back(p.parse(line.substr(6)));
					}
					else if (line == "end_spacegroup")
					{
						for (auto& cenop: cenops)
						{
							for (auto symop: symops)
							{
								symop = move_symop(symop, cenop);

								data.emplace_back(cur.nr, symopnr, symop);
								++symopnr;
							}
						}

						symInfo.emplace(cur.nr, cur);
						state = State::skip;

						symops.clear();
						cenops.clear();
					}
					break;
				}
			}
		}

		// --------------------------------------------------------------------

		sort(data.begin(), data.end());

		// --------------------------------------------------------------------

		out << R"(// This file was generated from $CLIBD/symop.lib
// and $CLIBD/syminfo.lib using symop-map-generator,
// part of the PDB-REDO suite of programs.

#include <cif++/Symmetry.hpp>

namespace mmcif
{

const Spacegroup kSpaceGroups[] =
{
)";

		std::vector<std::tuple<std::string,int,std::string,std::string>> spacegroups;

		for (auto& [nr, info]: symInfo)
		{
			spacegroups.emplace_back(info.old[0], nr, info.xHM, info.Hall);
			if (info.old[1].empty() == false)
				spacegroups.emplace_back(info.old[1], nr, info.xHM, info.Hall);
		}

		sort(spacegroups.begin(), spacegroups.end());

		for (auto [old, nr, xHM, Hall]: spacegroups)
		{
			old = '"' + old + '"' + std::string(20 - old.length(), ' ');
			xHM = '"' + xHM + '"' + std::string(30 - xHM.length(), ' ');

			for (std::string::size_type p = Hall.length(); p > 0; --p)
			{
				if (Hall[p - 1] == '"')
					Hall.insert(p - 1, "\\", 1);
			}

			Hall = '"' + Hall + '"' + std::string(40 - Hall.length(), ' ');

			out << "\t{ " << old << ", " << xHM << ", " << Hall << ", " << nr << " }," << std::endl;
		}

out << R"(
};

const size_t kNrOfSpaceGroups = sizeof(kSpaceGroups) / sizeof(Spacegroup);

const SymopDataBlock kSymopNrTable[] = {
)" << std::endl;

		int spacegroupNr = 0;
		for (auto& sd: data)
		{
			int sp, o;
			std::tie(sp, o, std::ignore) = sd;

			if (sp > spacegroupNr)
				out << "    // " << symInfo[sp].xHM << std::endl;
			spacegroupNr = sp;

			out << "    { " << std::setw(3) << sp
					<< ", " << std::setw(3) << o << ", { ";
			for (auto& i: std::get<2>(sd))
				out << std::setw(2) << i << ',';
			out << " } }," << std::endl;
		}

		out << R"(};

const size_t kSymopNrTableSize = sizeof(kSymopNrTable) / sizeof(SymopDataBlock);

} // namespace mmcif
)" << std::endl;

		out.close();
		fs::rename(tmpFile, output);
	}
	catch (const std::exception& ex)
	{
		std::cerr << std::endl
			 << "Program terminated due to error:" << std::endl
			 << ex.what() << std::endl;
	}
	
	return 0;
}
