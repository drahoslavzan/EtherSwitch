//===================================================================
// File:        cam.cc
// Author:      Drahoslav Zan
// Email:       izan@fit.vutbr.cz
// Affiliation: Brno University of Technology,
//              Faculty of Information Technology
// Date:        Tue Apr 24 19:11:10 CET 2012
// Comments:    CAM table
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


#include "cam.h"

#include <iomanip>
#include <cstring>
#include <cassert>


CAMTable CAMTable::cam;


CAMTable::CAMTable()
{
	memset(table, 0, sizeof(table));

	pthread_rwlock_init(&lock, NULL);

	setMinTTL(DEFAULT_MIN_TTL);

	for(size_t i = 1; i <= CAM_TABLE_SIZE; ++i)
		free.push(i);
}

CAMTable & CAMTable::instance()
{
	return cam;
}

void CAMTable::setMinTTL(unsigned sec)
{
	minTTL = sec;
}

void CAMTable::setDefaultPort(Port *p)
{
	table[0].port = p;
}

void CAMTable::insert(const MACAddr &mac, Port *p)
{
	if(mac.isBroadcast()) return;

	pthread_rwlock_wrlock(&lock);

	if(free.empty()) 				// full table
	{
		camlookup_t::const_iterator it = lookup.find(mac);

		if(it != lookup.end())
		{
			size_t i = it->second;

			table[i].port = p; 	// avoid port flapping
			table[i].timeStamp = time(NULL);
		}
	}
	else
	{
		size_t &i = lookup[mac];

		if(!i) 								// new address
		{
			i = free.front();
			free.pop();

			table[i].port = p;
			table[i].timeStamp = time(NULL);
		}
		else
		{
			table[i].port = p; 	// avoid port flapping
			table[i].timeStamp = time(NULL);
		}
	}

	pthread_rwlock_unlock(&lock);
}

Port * CAMTable::find(const MACAddr &mac)
{
	if(mac.isBroadcast())
		return &Broadcast::instance();

	Port *p;

	pthread_rwlock_rdlock(&lock);

	camlookup_t::const_iterator i = lookup.find(mac);

	if(i == lookup.end())
		p = table[0].port;
	else
	{
		p = table[i->second].port;
		table[i->second].timeStamp = time(NULL);
	}

	pthread_rwlock_unlock(&lock);

	return p;
}

void CAMTable::cleanup()
{
	pthread_rwlock_wrlock(&lock);

	time_t now = time(NULL);

	CAMTable::camlookup_t::iterator it = lookup.begin();
	for(; it != lookup.end(); ++it)
	{
		if(now - table[it->second].timeStamp >= minTTL)
		{
			free.push(it->second);
			lookup.erase(it);
		}
	}

	pthread_rwlock_unlock(&lock);
}

std::ostream & operator <<(std::ostream &os, const CAMTable &c)
{
	os << "MAC address\tPort\tAge";

	pthread_rwlock_rdlock(&c.lock);

	time_t now = time(NULL);

	CAMTable::camlookup_t::const_iterator it = c.lookup.begin();

	size_t i = 0;

	for(; it != c.lookup.end(); ++it, ++i)
		os << std::endl << it->first << '\t' << c.table[it->second].port->name()
			<< '\t' << (now - c.table[it->second].timeStamp);

	os << std::endl << "-- Total " << i << " / " << CAM_TABLE_SIZE << " --";

	pthread_rwlock_unlock(&c.lock);

	return os;
}

