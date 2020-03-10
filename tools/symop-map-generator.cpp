/*
cd ~/projects/pdb-redo/libcif++/tools/
clang++ -I ~/my-clipper/include -L ~/my-clipper/lib -o symop-map-generator symop-map-generator.cpp -lclipper-core
./symop-map-generator

*/

#include <cassert>

#include <iostream>
#include <iomanip>
#include <fstream>
#include <regex>
#include <map>

#include <cstdlib>

using namespace std;

std::regex kNameRx(R"(^(\d+) +(\d+) +(\d+) +(\S+) +(\S+) +(\S+) +'([^']+)'( +'([^']+)')?(?: +!.+)?$)");

class SymopParser
{
  public:
	SymopParser() {}

	array<int,15> parse(const string& s)
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
			throw runtime_error("symmetry expression contains more data than expected");

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

	string to_string(Token t)
	{
		switch (t)
		{
			case Eof:		return "end of expression";
			case Number:	return "number";
			case XYZ:		return "'x', 'y' or 'z'";
			default:
				if (isprint(t))
					return string({'\'', static_cast<char>(t), '\''});
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
			throw runtime_error("Unexpected character " + to_string(m_lookahead) + " expected " + to_string(token));
		
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

	string m_s;
	string::const_iterator m_p, m_e;

	int m_rot[3][3] = {};
	int m_trn[3][2] = {};
};

int main()
{
	try
	{
		const char* CLIBD = getenv("CLIBD");
		
		if (CLIBD == nullptr)
			throw runtime_error("CCP4 not sourced");

		// --------------------------------------------------------------------

		// store symop data here
		vector<tuple<int,int,array<int,15>>> data;

		// -----------------------------------------------------------------------
		
		struct SymInfoBlock
		{
			int nr;
			string xHM;
			string Hall;
			string old[2];
		};

		map<int,SymInfoBlock> symInfo;
		int symopnr, mysymnr = 10000;

		ifstream file(CLIBD + "/syminfo.lib"s);
		if (not file.is_open())
			throw runtime_error("Could not open syminfo.lib file");

		enum class State { skip, spacegroup } state = State::skip;

		string line;
		string Hall;
		vector<string> old;

		const regex rx(R"(^symbol +(Hall|xHM|old) +'(.+?)'(?: +'(.+?)')?$)"),
			rx2(R"(symbol ccp4 (\d+))");;

		SymInfoBlock cur = {};

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
					smatch m;
					if (regex_match(line, m, rx))
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
						data.emplace_back(cur.nr, symopnr, p.parse(line.substr(6)));
						++symopnr;
					}
					else if (line == "end_spacegroup")
					{
						symInfo.emplace(cur.nr, cur);
						state = State::skip;
					}
					break;
				}
			}
		}

		// // --------------------------------------------------------------------
		
		// string line;
		// string spacegroupName;
		// int spacegroupNr, symopnr;

		// enum class State { Initial, InSpacegroup, Error } state = State::Initial;

		// while (getline(file, line))
		// {
		// 	if (line.empty())
		// 		throw runtime_error("Invalid symop.lib file, contains empty line");
			
		// 	switch (state)
		// 	{
		// 		case State::Error:
		// 		case State::InSpacegroup:
		// 			if (line[0] == ' ')
		// 			{
		// 				if (state == State::Error)
		// 					continue;
						
		// 				try
		// 				{
		// 					SymopParser p;
		// 					data.emplace_back(spacegroupNr, symopnr, spacegroupName, p.parse(line));
		// 					++symopnr;
		// 				}
		// 				catch (const exception& e)
		// 				{
		// 					cerr << line << endl
		// 						 << e.what() << endl;
		// 				}

		// 				continue;
		// 			}
		// 			// fall through
				
		// 		case State::Initial:
		// 		{
		// 			smatch m;
		// 			if (not regex_match(line, m, kNameRx))
		// 			{
		// 				cerr << line << endl;
		// 				throw runtime_error("Name line does not match regular expression");
		// 			}

		// 			spacegroupNr = stoi(m[1]);
		// 			spacegroupName = m[7];
		// 			symopnr = 1;

		// 			if (not symInfo.count(spacegroupNr))
		// 				throw runtime_error("Symmetry nr not found in syminfo.lib");
					
		// 			if (symInfo[spacegroupNr].xHM != spacegroupName)
		// 				cerr << "Inconsistent data between symop.lib and syminfo.lib for spacegroup nr " << to_string(spacegroupNr) << endl;

		// 			state = State::InSpacegroup;
		// 			break;
		// 		}
		// 	}
		// }

		// --------------------------------------------------------------------

		sort(data.begin(), data.end());

		// --------------------------------------------------------------------

		cout << R"(// This file was generated from $CLIBD/symop.lib
// and $CLIBD/syminfo.lib using symop-map-generator,
// part of the PDB-REDO suite of programs.

struct Spacegroup
{
	const char* name;
	const char* xHM;
	const char* Hall;
	int nr;
} kSpaceGroups[] =
{
)";

		vector<tuple<string,int,string,string>> spacegroups;

		for (auto& [nr, info]: symInfo)
		{
			spacegroups.emplace_back(info.old[0], nr, info.xHM, info.Hall);
			if (info.old[1].empty() == false)
				spacegroups.emplace_back(info.old[1], nr, info.xHM, info.Hall);
		}

		sort(spacegroups.begin(), spacegroups.end());

		for (auto [old, nr, xHM, Hall]: spacegroups)
		{
			old = '"' + old + '"' + string(20 - old.length(), ' ');
			xHM = '"' + xHM + '"' + string(30 - xHM.length(), ' ');

			for (string::size_type p = Hall.length(); p > 0; --p)
			{
				if (Hall[p - 1] == '"')
					Hall.insert(p - 1, "\\", 1);
			}

			Hall = '"' + Hall + '"' + string(40 - Hall.length(), ' ');

			cout << "\t{ " << old << ", " << xHM << ", " << Hall << ", " << nr << " }," << endl;
		}

cout << R"(
};

union SymopData
{
	struct
	{
		int rot_0_0:2;
		int rot_0_1:2;
		int rot_0_2:2;
		int rot_1_0:2;
		int rot_1_1:2;
		int rot_1_2:2;
		int rot_2_0:2;
		int rot_2_1:2;
		int rot_2_2:2;
		unsigned int trn_0_0:3;
		unsigned int trn_0_1:3;
		unsigned int trn_1_0:3;
		unsigned int trn_1_1:3;
		unsigned int trn_2_0:3;
		unsigned int trn_2_1:3;
	};
	uint64_t iv:36;
};

struct SymopDataBlock
{
	uint16_t spacegroupNr;
	uint8_t rotationalNr;
	SymopData rt;
} kSymopNrTable[] = {
)" << endl;

		int spacegroupNr = 0;
		for (auto& sd: data)
		{
			int sp, o;
			tie(sp, o, ignore) = sd;

			if (sp > spacegroupNr)
				cout << "    // " << symInfo[sp].xHM << endl;
			spacegroupNr = sp;

			cout << "    { " << setw(3) << sp
					<< ", " << setw(3) << o << ", { { ";
			for (auto i: get<2>(sd))
				cout << setw(2) << i << ',';
			cout << " } } }," << endl;
		}

		cout << "};" << endl;
	}
	catch (const exception& ex)
	{
		cerr << endl
			 << "Program terminated due to error:" << endl
			 << ex.what() << endl;
	}
	
	return 0;
}
