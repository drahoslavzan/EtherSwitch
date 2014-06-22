//===================================================================
// File:        mac.h
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


#ifndef _MAC_H_
#define _MAC_H_

#include <sys/types.h>

#include <iostream>

class MACAddr
{
public:
	enum { LENGTH = 6 };
private:
	u_int8_t addr[LENGTH];
public:
	MACAddr();
	MACAddr(const u_int8_t *a);

	void set(const u_int8_t *a);

	operator const u_int8_t *() const;
	const MACAddr & operator =(const u_int8_t *a);
	bool operator <(const MACAddr &m) const;
	bool operator ==(const MACAddr &m) const;

	bool isBroadcast() const;
	bool isMulticast() const;

	friend std::ostream & operator <<(std::ostream &os, const MACAddr &m);
};

class EtherHeader
{
	MACAddr dst;
	MACAddr src;
	u_int16_t proto;
public:
	void set(const u_int8_t *frame);

	const MACAddr & destination() const;
	const MACAddr & source() const;
	u_int16_t type() const;
};


#endif /* _MAC_H_ */
