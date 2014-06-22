//===================================================================
// File:        main.cc
// Author:      Drahoslav Zan
// Email:       izan@fit.vutbr.cz
// Affiliation: Brno University of Technology,
//              Faculty of Information Technology
// Date:        Tue Apr 24 19:11:10 CET 2012
// Comments:    Program entry point
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
#include "port.h"
#include "cam.h"

#include <sys/types.h>

#include <libnet.h>

#include <cstdlib>
#include <cstring>

#include <iostream>
#include <string>
#include <cassert>


using namespace std;


#if DEFAULT_CAM_CLEANUP <= 0
# 	define DEFAULT_CAM_CLEANUP 		1
#endif

#define PROMPT 										"switch>"


extern char *optarg;
extern int optind, opterr, optopt;

static const char *progName;


void * cleaner(void *data)
{
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	CAMTable &cam = CAMTable::instance();
	MulticastStack &ms = MulticastStack::instance();

	for(;;)
	{
		sleep((long)data);
		cam.cleanup();
		ms.cleanup();
	}

	pthread_exit(NULL);
}

void snoopIGMP(Interface *iface, const u_int8_t *frame, size_t len)
{
	MulticastStack &mcstack = MulticastStack::instance();

	libnet_ipv4_hdr *ipv4 = (libnet_ipv4_hdr *)(frame + LIBNET_ETH_H);

	libnet_igmp_hdr *igmp = (libnet_igmp_hdr *)(frame + LIBNET_ETH_H
			+ 4 * ipv4->ip_hl);

	switch(igmp->igmp_type)
	{
		case IGMP_MEMBERSHIP_QUERY:
		{
			mcstack.sendQuery(iface, frame, len);

			break;
		}
		case IGMP_V1_MEMBERSHIP_REPORT:
		case IGMP_V2_MEMBERSHIP_REPORT:
		{
			if(iface->same(mcstack.getQuerier()))
				break;

			Multicast *mc = mcstack[igmp->igmp_group.s_addr];

			if(mc != NULL)
				mc->add(iface);

			mcstack.sendResponse(frame, len, iface);

			break;
		}
		case IGMP_LEAVE_GROUP:
		{
			Multicast *mc = mcstack.find(igmp->igmp_group.s_addr);

			if(mc != NULL) mc->remove(iface);


			// !!! SEND TO QUERIER and OTHERS ??? !!!
			// !!! SEND TO QUERIER and OTHERS ??? !!!
			// !!! SEND TO QUERIER and OTHERS ??? !!!
			// !!! SEND TO QUERIER and OTHERS ??? !!!
			// !!! SEND TO QUERIER and OTHERS ??? !!!
			// !!! SEND TO QUERIER and OTHERS ??? !!!
			// !!! SEND TO QUERIER and OTHERS ??? !!!

			break;
		}
		default:
		{
			if(iface->same(mcstack.getQuerier()))
				break;

			Multicast *mc = mcstack[igmp->igmp_group.s_addr];

			if(mc != NULL)
				mc->add(iface);

			break;
		}
	}
}

void * traffic(void *data)
{
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	CAMTable &cam = CAMTable::instance();
	InterfaceStack &ifs = InterfaceStack::instance();
	MulticastStack &mcstack = MulticastStack::instance();

	Interface *iface = ifs[(long)data];

	assert(iface != NULL);

	for(;;)
	{
		size_t len;
		const u_int8_t *frame;
		EtherHeader eth;
	 
		if((frame = iface->recv(&len)) == NULL)
			continue;

		eth.set(frame);

		// IPv4 MULTICAST
		if(eth.destination().isMulticast() && eth.type() == ETHERTYPE_IP)
		{
			libnet_ipv4_hdr *ipv4 = (libnet_ipv4_hdr *)(frame + LIBNET_ETH_H);

			if(ipv4->ip_v == 4)
			{
				// IGMP SNOOPING
				if(ipv4->ip_p == IPPROTO_IGMP)
					snoopIGMP(iface, frame, len);
				else
				{
					u_int32_t group = ipv4->ip_dst.s_addr;

					Multicast *mc = mcstack.find(group);

					if(mc != NULL)
						mc->send(frame, len, iface);
					else
						Broadcast::instance().send(frame, len, iface);
				}
			}
		}
		// UNICAST, BROADCAST
		else
		{
			cam.insert(eth.source(), iface);
			Port *p = cam.find(eth.destination());

			p->send(frame, len, iface);
		}
	}

	pthread_exit(NULL);
}


void usage(ostream &stream, int ecode)
{
	stream << "USAGE: " << progName << " [OPTIONS]" << endl;
	stream << "Type `" << progName << " -h' form more information." << endl;

	exit(ecode);
}

void help(ostream &stream, int ecode)
{
	stream << "USAGE: " << progName << " [OPTIONS]" << endl;
	stream << endl;
	stream << "OPTIONS:" << endl;
	stream << "    -t sec    Minimum time to live for CAM table entry ("
		<< DEFAULT_MIN_TTL << ")" << endl;
	stream << "    -c sec    CAM table cleanup interval ("
		<< DEFAULT_CAM_CLEANUP << ")" << endl;
	stream << "    -h        Show this help and exit" << endl;
	stream << endl;
	stream << "Software switch with multicast support.";
	stream << endl;

	exit(ecode);
}

int main(int argc, char **argv)
{
	progName = argv[0];

	// OPTIONS

	int optMinTTL = DEFAULT_MIN_TTL;
	int optCleanup = DEFAULT_CAM_CLEANUP;

	int opt;
	while((opt = getopt(argc, argv, "t:c:h")) != -1)
		switch(opt)
		{
			case 't':
				optMinTTL = atoi(optarg);
				break;
			case 'c':
				optCleanup = atoi(optarg);
				break;
			case 'h':
				help(cout, 0);
			case '?':
				usage(cerr, 1);
		}

	if(optMinTTL <= 0)
	{
		cerr << "ERROR: Invalid argument for -t parameter" << endl;
		return 1;
	}
	else if(optCleanup <= 0)
	{
		cerr << "ERROR: Invalid argument for -c parameter" << endl;
		return 1;
	}

	CAMTable &cam = CAMTable::instance();

	cam.setDefaultPort(&Broadcast::instance());
	cam.setMinTTL(optMinTTL);

	// INTERFACES

	InterfaceStack &ifs = InterfaceStack::instance();

	if(!ifs.error().empty())
	{
		cerr << "ERROR: " << ifs.error() << endl;
		return 1;
	}

	size_t nthrds = ifs.size();

	if(nthrds < 2)
	{
		cerr << "ERROR: Found " << nthrds << " interfaces -- minimum 2" << endl;
		return 1;
	}

	// THREADS

	pthread_t clnr;
	pthread_t *threads = new pthread_t[nthrds];

	int rc;

	if((rc = pthread_create(&clnr, NULL, cleaner, (void *)((long)optCleanup))))
	{
		cerr << "ERROR: pthread_create(): " << strerror(rc) << endl;
		return 1;
	}

	for(unsigned long i = 0; i < nthrds; ++i)
		if((rc = pthread_create(&threads[i], NULL, traffic, (void *)i)))
		{
			cerr << "ERROR: pthread_create(): " << strerror(rc) << endl;
			return 1;
		}

	// COMMAND LINE

	MulticastStack &igmp = MulticastStack::instance();

	string cmd;

	for(;;)
	{
		cout << PROMPT << " ";

		getline(cin, cmd);

		if(!cin.good())
		{
			cout << "quit" << endl;
			break;
		}

		if(cmd.empty())
			continue;
		else if(cmd == "stat")
			cout << ifs << endl << endl;
		else if(cmd == "cam")
			cout << cam << endl << endl;
		else if(cmd == "igmp")
			cout << igmp << endl << endl;
		else if(cmd == "quit")
			break;
		else if(cmd == "help")
		{
			cout << "CMD     DESCRIPTION" << endl;
			cout << "------------------------------" << endl;
			cout << "stat    Show interface stats" << endl;
			cout << "cam     Show CAM table content" << endl;
			cout << "igmp    Show multicast info" << endl;
			cout << "help    Show this help" << endl;
			cout << "quit    Exit" << endl;
			cout << endl;
		}
		else
			cerr << "ERROR: Invalid command `" << cmd << "'" << endl << endl;
	}

	pthread_cancel(clnr);

	for(unsigned long i = 0; i < nthrds; ++i)
		pthread_cancel(threads[i]);

	delete [] threads;

	//pthread_exit(NULL);

	return 0;
}

