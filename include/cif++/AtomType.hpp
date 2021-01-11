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

// Lib for working with structures as contained in mmCIF and PDB files

#pragma once

#include <cstdint>
#include <string>
#include <stdexcept>

namespace mmcif
{

enum AtomType : uint8_t
{
	Nn = 0,		// Unknown
	
	H = 1,		// Hydro­gen
	He = 2,		// He­lium

	Li = 3,		// Lith­ium
	Be = 4,		// Beryl­lium
	B = 5,		// Boron
	C = 6,		// Carbon
	N = 7,		// Nitro­gen
	O = 8,		// Oxy­gen
	F = 9,		// Fluor­ine
	Ne = 10,	// Neon

	Na = 11,	// So­dium
	Mg = 12,	// Magne­sium
	Al = 13,	// Alumin­ium
	Si = 14,	// Sili­con
	P = 15,		// Phos­phorus
	S = 16,		// Sulfur
	Cl = 17,	// Chlor­ine
	Ar = 18,	// Argon

	K = 19,		// Potas­sium
	Ca = 20,	// Cal­cium
	Sc = 21,	// Scan­dium
	Ti = 22,	// Tita­nium
	V = 23,		// Vana­dium
	Cr = 24,	// Chrom­ium
	Mn = 25,	// Manga­nese
	Fe = 26,	// Iron
	Co = 27,	// Cobalt
	Ni = 28,	// Nickel
	Cu = 29,	// Copper
	Zn = 30,	// Zinc
	Ga = 31,	// Gallium
	Ge = 32,	// Germa­nium
	As = 33,	// Arsenic
	Se = 34,	// Sele­nium
	Br = 35,	// Bromine
	Kr = 36,	// Kryp­ton

	Rb = 37,	// Rubid­ium
	Sr = 38,	// Stront­ium
	Y = 39,		// Yttrium
	Zr = 40,	// Zirco­nium
	Nb = 41,	// Nio­bium
	Mo = 42,	// Molyb­denum
	Tc = 43,	// Tech­netium
	Ru = 44,	// Ruthe­nium
	Rh = 45,	// Rho­dium
	Pd = 46,	// Pallad­ium
	Ag = 47,	// Silver
	Cd = 48,	// Cad­mium
	In = 49,	// Indium
	Sn = 50,	// Tin
	Sb = 51,	// Anti­mony
	Te = 52,	// Tellurium
	I = 53,		// Iodine
	Xe = 54,	// Xenon
	Cs = 55,	// Cae­sium
	Ba = 56,	// Ba­rium
	La = 57,	// Lan­thanum

	Hf = 72,	// Haf­nium
	Ta = 73,	// Tanta­lum
	W = 74,		// Tung­sten
	Re = 75,	// Rhe­nium
	Os = 76,	// Os­mium
	Ir = 77,	// Iridium
	Pt = 78,	// Plat­inum
	Au = 79,	// Gold
	Hg = 80,	// Mer­cury
	Tl = 81,	// Thallium
	Pb = 82,	// Lead
	Bi = 83,	// Bis­muth
	Po = 84,	// Polo­nium
	At = 85,	// Asta­tine
	Rn = 86,	// Radon
	Fr = 87,	// Fran­cium
	Ra = 88,	// Ra­dium
	Ac = 89,	// Actin­ium

	Rf = 104,	// Ruther­fordium
	Db = 105,	// Dub­nium
	Sg = 106,	// Sea­borgium
	Bh = 107,	// Bohr­ium
	Hs = 108,	// Has­sium
	Mt = 109,	// Meit­nerium
	Ds = 110,	// Darm­stadtium
	Rg = 111,	// Roent­genium
	Cn = 112,	// Coper­nicium
	Nh = 113,	// Nihon­ium
	Fl = 114,	// Flerov­ium
	Mc = 115,	// Moscov­ium
	Lv = 116,	// Liver­morium
	Ts = 117,	// Tenness­ine
	Og = 118,	// Oga­nesson

	Ce = 58,	// Cerium
	Pr = 59,	// Praseo­dymium
	Nd = 60,	// Neo­dymium
	Pm = 61,	// Prome­thium
	Sm = 62,	// Sama­rium
	Eu = 63,	// Europ­ium
	Gd = 64,	// Gadolin­ium
	Tb = 65,	// Ter­bium
	Dy = 66,	// Dyspro­sium
	Ho = 67,	// Hol­mium
	Er = 68,	// Erbium
	Tm = 69,	// Thulium
	Yb = 70,	// Ytter­bium
	Lu = 71,	// Lute­tium

	Th = 90,	// Thor­ium
	Pa = 91,	// Protac­tinium
	U = 92,		// Ura­nium
	Np = 93,	// Neptu­nium
	Pu = 94,	// Pluto­nium
	Am = 95,	// Ameri­cium
	Cm = 96,	// Curium
	Bk = 97,	// Berkel­ium
	Cf = 98,	// Califor­nium
	Es = 99,	// Einstei­nium
	Fm = 100,	// Fer­mium
	Md = 101,	// Mende­levium
	No = 102,	// Nobel­ium
	Lr = 103,	// Lawren­cium

	D = 129,	// Deuterium
};

// --------------------------------------------------------------------
// AtomTypeInfo

enum RadiusType {
	eRadiusCalculated,
	eRadiusEmpirical,
	eRadiusCovalentEmpirical,

	eRadiusSingleBond,
	eRadiusDoubleBond,
	eRadiusTripleBond,

	eRadiusVanderWaals,

	eRadiusTypeCount
};

struct AtomTypeInfo
{
	AtomType		type;
	std::string		name;
	std::string		symbol;
	float			weight;
	bool			metal;
	float			radii[eRadiusTypeCount];
};

extern const AtomTypeInfo kKnownAtoms[];

// --------------------------------------------------------------------
// AtomTypeTraits

class AtomTypeTraits
{
  public:
	AtomTypeTraits(AtomType a);
	AtomTypeTraits(const std::string& symbol);
	
	AtomType type() const			{ return mInfo->type; }
	std::string	name() const		{ return mInfo->name; }
	std::string	symbol() const		{ return mInfo->symbol; }
	float weight() const			{ return mInfo->weight; }
	
	bool isMetal() const			{ return mInfo->metal; }
	
	static bool isElement(const std::string& symbol);
	static bool isMetal(const std::string& symbol);
	
	float radius(RadiusType type = eRadiusSingleBond) const
	{
		if (type >= eRadiusTypeCount)
			throw std::invalid_argument("invalid radius requested");
		return mInfo->radii[type] / 100.f;
	}
	
	// data type encapsulating the Waasmaier & Kirfel scattering factors
	// in a simplified form (only a and b).
	// Added the electrion scattering factors as well
	struct SFData
	{
		double a[6], b[6];
	};
	
	// to get the Cval and Siva values, use this constant as charge:
	enum { kWKSFVal = -99 };
	
	const SFData& wksf(int charge = 0) const;
	const SFData& elsf() const;

  private:
	const struct AtomTypeInfo*	mInfo;
};

}
