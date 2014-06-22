//===================================================================
// File:        port.cc
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


#include "port.h"

#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>

#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <libnet.h>

#include <cassert>
#include <cstring>


pthread_mutex_t Port::mutexId = PTHREAD_MUTEX_INITIALIZER;
int Port::freeId = 0;

Interface * Multicast::qrr = NULL;

InterfaceStack InterfaceStack::ifs;

Broadcast Broadcast::brc;

MulticastStack MulticastStack::mst;

inline static bool VALID_DEVICE(const char *name, unsigned flags)
{
	if(flags == PCAP_IF_LOOPBACK) return false;

	//if(!strcmp(name, "any")) return false;
	//if(!strncmp(name, "usbmon", 6)) return false;

	struct ifreq ifr;

	memset(&ifr, 0, sizeof(ifr));

	int s = socket(AF_INET, SOCK_DGRAM, 0);

	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, name, IFNAMSIZ-1);

	ioctl(s, SIOCGIFHWADDR, &ifr);

	close(s);

	if(!memcmp(ifr.ifr_hwaddr.sa_data, "\0\0\0\0\0\0", 6))
		return false;

	return true;

	// NOT WORKING CORRECTLY IN VIRTUALBOX
#if 0
	char errbuf[LIBNET_ERRBUF_SIZE];

	libnet_t *l = libnet_init(LIBNET_LINK, name, errbuf);
	if(l == NULL) return false;

	libnet_ether_addr *mac = libnet_get_hwaddr(l);
	if(mac == NULL)
	{
		libnet_destroy(l);
		return false;
	}

	libnet_destroy(l);
	return true;
#endif
}


int Port::assignId()
{
	pthread_mutex_lock(&mutexId);

	int i = freeId++;

	pthread_mutex_unlock(&mutexId);

	return i;
}

Port::Port()
:
	id(freeId++)
{
}

bool Port::same(const Port *p)
{
	if(p == NULL)
		return false;

	return id == p->id;
}


const u_int8_t * Interface::receive()
{
	const u_int8_t *data = pcap_next(fp, &hdr);

	assert(hdr.caplen == hdr.len); 		// avoid data loss

	++recvF;
	recvB += hdr.len;

	return data;
}

Interface::Interface(const char *nm)
:
	recvB(0), sentB(0), recvF(0), sentF(0),
	opened(false), ifnm(nm)
{
	char perr[PCAP_ERRBUF_SIZE];

	pthread_mutex_init(&mutexSent, NULL);

	if((fp = pcap_open_live(nm, SNAPLEN, 1, 0, perr)) == NULL)
		throw std::string("Interface(): pcap_open_live(): ") + perr;

	//if(pcap_setdirection(fp, PCAP_D_IN) == -1)
		//throw std::string("Interface(): pcap_set_direction(): ") + pcap_geterr(fp);

	opened = true;
}

Interface::~Interface()
{
	if(opened) pcap_close(fp);
}

void Interface::send(const u_int8_t *frame, size_t len, const Port *in)
{
	assert(in != NULL);

	if(same(in))
		return;

	send(frame, len);
}

void Interface::send(const u_int8_t *frame, size_t len)
{
	pthread_mutex_lock(&mutexSent);

	++sentF;
	sentB += len;

	pthread_mutex_unlock(&mutexSent);

	pcap_sendpacket(fp, frame, len); 	// auto synchronized
}

const u_int8_t * Interface::recv(size_t *len)
{
	const u_int8_t *data = receive();

	if(data == NULL) return NULL;

	*len = hdr.len;

	return data;
}

const char * Interface::name() const
{
	return ifnm.c_str();
}

unsigned long Interface::statSentBytes() const
{
	return sentB;
}

unsigned long Interface::statSentFrames() const
{
	return sentF;
}

unsigned long Interface::statRecvBytes() const
{
	return recvB;
}

unsigned long Interface::statRecvFrames() const
{
	return recvF;
}


Broadcast::Broadcast()
{
}

Broadcast & Broadcast::instance()
{
	return brc;
}

void Broadcast::send(const u_int8_t *frame, size_t len, const Port *in)
{
	InterfaceStack &ifs = InterfaceStack::instance();

	for(size_t i = 0; i < ifs.size(); ++i)
		ifs[i]->send(frame, len, in);
}

void Broadcast::send(const u_int8_t *frame, size_t len)
{
	InterfaceStack &ifs = InterfaceStack::instance();

	for(size_t i = 0; i < ifs.size(); ++i)
		ifs[i]->send(frame, len);
}

const char * Broadcast::name() const
{
	return "<bcast>";
}


InterfaceStack & InterfaceStack::instance()
{
	return ifs;
}

InterfaceStack::InterfaceStack()
:
	err("")
{
	char perr[PCAP_ERRBUF_SIZE];
	pcap_if_t *alldevs;

	if(pcap_findalldevs(&alldevs, perr) == -1)
	{
		err = "InterfaceStack(): pcap_findalldevs(): ";
		err += perr;
	}

	try
	{
		for(pcap_if_t *d = alldevs; d != NULL; d = d->next)
			if(VALID_DEVICE(d->name, d->flags))
					table.push_back(new Interface(d->name));
	}
	catch(const std::string &str)
	{
		err = "InterfaceStack(): ";
		err += str;
	}

	pcap_freealldevs(alldevs);
}

InterfaceStack::~InterfaceStack()
{
	std::vector<Interface *>::iterator it = table.begin();

	for(; it != table.end(); ++it)
		delete *it;
}

const std::string & InterfaceStack::error() const
{
	return err;
}

size_t InterfaceStack::size() const
{
	return table.size();
}

Interface * InterfaceStack::operator [](size_t i) const
{
	assert(i <= table.size());

	return table[i];
}

std::ostream & operator <<(std::ostream &os, const InterfaceStack &s)
{
	os << "Iface\t\tSent-B\t\tSent-frm\tRecv-B\t\tRecv-frm";

	std::vector<Interface *>::const_iterator it = s.table.begin();

	for(; it != s.table.end(); ++it)
		os << std::endl << (*it)->name() << "\t\t" << (*it)->statSentBytes()
			<< "\t\t" << (*it)->statSentFrames() << "\t\t" << (*it)->statRecvBytes()
			<< "\t\t" << (*it)->statRecvFrames();

	return os;
}


void Multicast::setQuerier(Interface *querier)
{
	qrr = querier;
}

Multicast::Multicast(u_int32_t group)
:
	grp(group)
{
	pthread_rwlock_init(&lock, NULL);
}

Multicast::~Multicast()
{
}

void Multicast::add(Interface *p)
{
	pthread_rwlock_wrlock(&lock);

	table.insert(p);

	pthread_rwlock_unlock(&lock);
}

void Multicast::remove(Interface *p)
{
	pthread_rwlock_wrlock(&lock);

	table.erase(p);

	pthread_rwlock_unlock(&lock);

	MulticastStack::instance().cleanup();
}

void Multicast::send(const u_int8_t *frame, size_t len, const Port *in)
{
	assert(qrr != NULL);

	pthread_rwlock_rdlock(&lock);

	std::set<Interface *>::const_iterator it = table.begin();

	qrr->send(frame, len, in);

	for(; it != table.end(); ++it)
		(*it)->send(frame, len, in);

	pthread_rwlock_unlock(&lock);
}

void Multicast::send(const u_int8_t *frame, size_t len)
{
	assert(qrr != NULL);

	pthread_rwlock_rdlock(&lock);

	std::set<Interface *>::const_iterator it = table.begin();

	qrr->send(frame, len);

	for(; it != table.end(); ++it)
		(*it)->send(frame, len);

	pthread_rwlock_unlock(&lock);
}

const char * Multicast::name() const
{
	return "<mcast>";
}

bool Multicast::empty() const
{
	return table.empty();
}

std::ostream & operator <<(std::ostream &os, const Multicast &m)
{
	assert(m.qrr != NULL);

	pthread_rwlock_rdlock(&m.lock);

	os << libnet_addr2name4(m.grp, LIBNET_DONT_RESOLVE) << "\t*" << m.qrr->name();

	std::set<Interface *>::const_iterator it = m.table.begin();

	for(; it != m.table.end(); ++it)
		os << ", " << (*it)->name();

	pthread_rwlock_unlock(&m.lock);

	return os;
}


MulticastStack::MulticastStack()
:
	qr(NULL)
{
	pthread_rwlock_init(&lock, NULL);
}

MulticastStack & MulticastStack::instance()
{
	return mst;
}

MulticastStack::~MulticastStack()
{
	mclookup_t::const_iterator it = table.begin();

	for(; it != table.end(); ++it)
		delete it->second;
}

void MulticastStack::sendQuery(Interface *querier, const u_int8_t *frame,
		size_t len)
{
	qr = querier;

	Multicast::setQuerier(qr);

	Broadcast::instance().send(frame, len, querier);
}

void MulticastStack::sendResponse(const u_int8_t *frame, size_t len,
		const Port *in)
{
	if(qr == NULL) return;

	qr->send(frame, len, in);
}

Multicast * MulticastStack::find(u_int32_t group) const
{
	pthread_rwlock_rdlock(&lock);

	mclookup_t::const_iterator it = table.find(group);

	if(it == table.end())
	{
		pthread_rwlock_unlock(&lock);
		return NULL;
	}

	pthread_rwlock_unlock(&lock);
	return it->second;
}

Multicast * MulticastStack::operator [](u_int32_t group)
{
	if(qr == NULL) return NULL;

	pthread_rwlock_wrlock(&lock);

	Multicast *&mc = table[group];

	if(mc == NULL) mc = new Multicast(group);

	mc->setQuerier(qr);

	pthread_rwlock_unlock(&lock);

	return mc;
}

Interface * MulticastStack::getQuerier() const
{
	return qr;
}

void MulticastStack::cleanup()
{
	pthread_rwlock_wrlock(&lock);

	mclookup_t::iterator it = table.begin();

	for(; it != table.end(); ++it)
		if(it->second->empty())
		{
			table.erase(it);
			delete it->second;
		}

	pthread_rwlock_unlock(&lock);
}

std::ostream & operator <<(std::ostream &os, const MulticastStack &s)
{
	os << "GroupAddr\tIfaces";

	pthread_rwlock_rdlock(&s.lock);

	MulticastStack::mclookup_t::const_iterator it = s.table.begin();

	for(; it != s.table.end(); ++it)
		os << std::endl << (*it->second);

	pthread_rwlock_unlock(&s.lock);

	return os;
}

