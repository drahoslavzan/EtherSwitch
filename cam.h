//===================================================================
// File:        cam.h
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


#ifndef _CAM_H_
#define _CAM_H_


#include "mac.h"
#include "port.h"

#include <pthread.h>

#include <ctime>
#include <cstdlib>

#include <iostream>
#include <map>
#include <queue>


#if CAM_TABLE_SIZE <= 0
# 	define CAM_TABLE_SIZE 				512
#endif

#if DEFAULT_MIN_TTL <= 0
# 	define DEFAULT_MIN_TTL 				300
#endif


class CAMTable
{
private:
	typedef std::map<MACAddr, size_t> camlookup_t;
	typedef std::queue<size_t> camfree_t;
private:
	struct Entry
	{
		Port *port;
		time_t timeStamp;
	};
private:
	static CAMTable cam;
private:
	mutable pthread_rwlock_t lock;
private:
	Entry table[CAM_TABLE_SIZE + 1]; 	// table[0] reserved
	camlookup_t lookup;
	camfree_t free;
private:
	time_t minTTL; 									// seconds
private:
	CAMTable();
public: 	// static //
	static CAMTable & instance();
public: 	// init //
	void setMinTTL(unsigned sec);
	void setDefaultPort(Port *p);
public: 	// non-concurrent //
	friend std::ostream & operator <<(std::ostream &os, const CAMTable &c);
public: 	// concurrent //
	void insert(const MACAddr &mac, Port *p);
	Port * find(const MACAddr &mac);

	void cleanup(); 									// periodically called
};


#endif /* _CAM_H_ */
