//===================================================================
// File:        port.h
// Author:      Drahoslav Zan
// Email:       izan@fit.vutbr.cz
// Affiliation: Brno University of Technology,
//              Faculty of Information Technology
// Date:        Tue Apr 24 19:11:10 CET 2012
// Comments:    Switch ports
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


#ifndef _PORT_H_
#define _PORT_H_


#include <sys/types.h>

#include <pcap.h>
#include <libnet.h>
#include <pthread.h>

#include <vector>
#include <map>
#include <set>
#include <string>
#include <iostream>


class Port
{
private:
	static pthread_mutex_t mutexId;
	static int freeId;
private:
	int assignId();
private:
	int id;
public:
	Port();

	virtual void send(const u_int8_t *frame, size_t len, const Port *in) = 0;
	virtual void send(const u_int8_t *frame, size_t len) = 0;

	virtual const char *name() const = 0;

	bool same(const Port *p);
};

class Interface : public Port
{
	enum { SNAPLEN = IP_MAXPACKET + LIBNET_ETH_H };
private:
	unsigned long recvB, sentB;
	unsigned long recvF, sentF;
private:
	pthread_mutex_t mutexSent;
private:
	bool opened;
	pcap_t *fp;
	pcap_pkthdr hdr;
	std::string ifnm;
public:
	const u_int8_t * receive();
public:
	Interface(const char *nm);
	virtual ~Interface();
public: // concurrent (boradcast) //
	void send(const u_int8_t *frame, size_t len, const Port *in);
	void send(const u_int8_t *frame, size_t len);
public: // non-concurrent
	const u_int8_t * recv(size_t *len);

	const char *name() const;

	unsigned long statSentBytes() const;
	unsigned long statSentFrames() const;
	unsigned long statRecvBytes() const;
	unsigned long statRecvFrames() const;
};

class Broadcast : public Port
{
private:
	static Broadcast brc;
private:
	Broadcast();
public:
	static Broadcast & instance();

	void send(const u_int8_t *frame, size_t len, const Port *in);
	void send(const u_int8_t *frame, size_t len);

	const char *name() const;
};

class InterfaceStack
{
private:
	static InterfaceStack ifs;
private:
	std::vector<Interface *> table;
	std::string err;
private:
	InterfaceStack();
public:
	static InterfaceStack & instance();
public:
	~InterfaceStack();

	const std::string & error() const;

	size_t size() const;
	Interface * operator [](size_t i) const;

	friend std::ostream & operator <<(std::ostream &os, const InterfaceStack &s);
};





class Multicast : public Port
{
private:
	static Interface *qrr;
private:
	std::set<Interface *> table;
	u_int32_t grp;
private:
	mutable pthread_rwlock_t lock;
public:
	static void setQuerier(Interface *querier);
public:
	Multicast(u_int32_t group);
	virtual ~Multicast();

	void add(Interface *p);
	void remove(Interface *p);

	void send(const u_int8_t *frame, size_t len, const Port *in);
	void send(const u_int8_t *frame, size_t len);

	const char *name() const;
	bool empty() const;

	friend std::ostream & operator <<(std::ostream &os, const Multicast &m);
};

// CONCURRENT !!!
class MulticastStack
{
	typedef std::map<u_int32_t, Multicast *> mclookup_t;
private:
	static MulticastStack mst;
	MulticastStack();
private:
	mutable pthread_rwlock_t lock;
private:
	mclookup_t table;
	Interface *qr;
public:
	static MulticastStack & instance();
public:
	~MulticastStack();

	void sendQuery(Interface *querier, const u_int8_t *frame, size_t len);
	void sendResponse(const u_int8_t *frame, size_t len, const Port *in);

	Multicast * find(u_int32_t group) const;
	Multicast * operator [](u_int32_t group);

	Interface * getQuerier() const;

	void cleanup();

	friend std::ostream & operator <<(std::ostream &os, const MulticastStack &s);
};


#endif /* _PORT_H_ */
