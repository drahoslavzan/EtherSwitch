//===================================================================
// File:        mac.cc
// Author:      Drahoslav Zan
// Email:       izan@fit.vutbr.cz
// Affiliation: Brno University of Technology,
//              Faculty of Information Technology
// Date:        Tue Apr 24 19:11:10 CET 2012
// Comments:    MAC address
// Project:     Software Ethernet Switch (SES)
//-------------------------------------------------------------------
// Copyright (C) 2013 Drahoslav Zan
//
// This file is part of SES.
//
// SES is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// SES is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with SES. If not, see <http://www.gnu.org/licenses/>.
//===================================================================
// vim: set nowrap sw=2 ts=2


#include "mac.h"

#include <netinet/in.h>

#include <iomanip>
#include <cstring>


MACAddr::MACAddr()
{
	memset(addr, 0, sizeof(addr));
}

MACAddr::MACAddr(const u_int8_t *a)
{
	set(a);
}

void MACAddr::set(const u_int8_t *a)
{
	memcpy(addr, a, sizeof(addr));
}

MACAddr::operator const u_int8_t *() const
{
	return addr;
}

const MACAddr & MACAddr::operator =(const u_int8_t *a)
{
	set(a);

	return *this;
}

bool MACAddr::operator <(const MACAddr &m) const
{
#if 0
	size_t i = 0;

	for(; i < sizeof(addr) - 1; ++i)
		if(addr[i] > m.addr[i]) return false;
		else if(addr[i] < m.addr[i]) return true;

	return (addr[i] < m.addr[i]) ? true : false;
#endif

	return memcmp(addr, m.addr, MACAddr::LENGTH) < 0;
}

bool MACAddr::operator ==(const MACAddr &m) const
{
#if 0
	for(size_t i = 0; i < sizeof(addr); ++i)
		if(addr[i] != m.addr[i]) return false;

	return true;
#endif

	return !memcmp(addr, m.addr, MACAddr::LENGTH);
}

bool MACAddr::isBroadcast() const
{
	for(size_t i = 0; i < sizeof(addr); ++i)
		if(addr[i] != 0xFF) return false;

	return true;
}

bool MACAddr::isMulticast() const
{
	return (addr[0] == 0x01 && addr[1] == 0x00 && addr[2] == 0x5E);
}

std::ostream & operator <<(std::ostream &os, const MACAddr &m)
{
	std::ios_base::fmtflags flags = os.setf(std::ios::hex, std::ios::basefield);

	os << std::setw(2) << std::setfill('0')
		 << (unsigned)m.addr[0]
		 << std::setw(2) << std::setfill('0')
		 << (unsigned)m.addr[1] << '.'
		 << std::setw(2) << std::setfill('0')
		 << (unsigned)m.addr[2]
		 << std::setw(2) << std::setfill('0')
		 << (unsigned)m.addr[3] << '.'
		 << std::setw(2) << std::setfill('0')
		 << (unsigned)m.addr[4]
		 << std::setw(2) << std::setfill('0')
		 << (unsigned)m.addr[5];

	os.setf(flags);

	return os;
}


void EtherHeader::set(const u_int8_t *frame)
{
	dst = frame;
	src = frame + MACAddr::LENGTH;
	proto = ntohs(*(frame + 2 * MACAddr::LENGTH));
}

const MACAddr & EtherHeader::destination() const
{
	return dst;
}

const MACAddr & EtherHeader::source() const
{
	return src;
}

u_int16_t EtherHeader::type() const
{
	return proto;
}
