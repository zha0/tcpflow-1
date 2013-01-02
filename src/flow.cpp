/**
 *
 * flow.cpp:
 *
 * The flow class is used to track individual TCP/IP flows (2 per connection).
 * The class implements the methods that turn a flow into a filename.
 *
 * This file is part of tcpflow by Jeremy Elson <jelson@circlemud.org>
 * Initial Release: 7 April 1999.
 *
 * This source code is under the GNU Public License (GPL).  See
 * LICENSE for details.
 *
 */

#include "tcpflow.h"
#include "tcpip.h"

#include <assert.h>
#include <iostream>
#include <sstream>

#define ETH_ALEN 6
#ifndef HAVE_INET_NTOP
#include "inet_ntop.c"
#endif

std::string flow::filename_template("%A.%a-%B.%b%V%v%C%c");
std::string flow::outdir(".");

void flow::usage()
{
    std::cout << "Filename template format:\n";
    std::cout << "  %A/%a - source IP address/port;          %B/%b - dest IP address/port\n";
    std::cout << "  %c/%# - connection_count;                %C - 'c' if connection_count >0\n";
    std::cout << "  %V/%v - VLAN number, '--' if no vlan/'' if no vlan\n";
    std::cout << "  %T/%t - Timestamp in ISO8601 format/unix time_t\n";
    std::cout << "  %% - Output a '%'\n";
    std::cout << "\n";
}

std::string flow::filename(uint32_t connection_count)
{
    bool used_cc=false;
    std::stringstream ss;

    /* Add the outdir */
    if(flow::outdir!="." && flow::outdir!=""){
        ss << flow::outdir;
        ss << '/';
    }

    for(unsigned int i=0;i<filename_template.size();i++){
	switch(filename_template.at(i)){
	default:
	    ss << filename_template.at(i);
	    break;
	case '%':
	    if(i==filename_template.size()-1){
		std::cerr << "Invalid filename_template: " << filename_template << " cannot end with a %\n";
		exit(1);
	    }
	    /* put the substitute in ss or buf */
	    char buf[1024];
	    buf[0] = 0;
	    switch(filename_template.at(++i)){
	    case 'A': // source IP address
		switch(family){
		case AF_INET:
		    snprintf(buf,sizeof(buf),"%03d.%03d.%03d.%03d", src.addr[0], src.addr[1], src.addr[2], src.addr[3]);
		    break;
		case AF_INET6:
		    inet_ntop(family, src.addr, buf,sizeof(buf));
		}
		break;
	    case 'a': // source IP port
		snprintf(buf,sizeof(buf),"%05d",sport);
		break;
	    case 'B': // dest IP address
		switch(family){
		case AF_INET:
		    snprintf(buf,sizeof(buf),"%03d.%03d.%03d.%03d", dst.addr[0], dst.addr[1], dst.addr[2], dst.addr[3]);
		    break;
		case AF_INET6:
		    inet_ntop(family, dst.addr, buf,sizeof(buf));
		}
		break;
	    case 'b': // dest IP port
		snprintf(buf,sizeof(buf),"%05d",dport);
		break;
	    case 'T': // Timestamp in ISO8601 format
	      {
		time_t t = tstart.tv_sec;
		strftime(buf,sizeof(buf),"%Y-%m-%dT%H:%M:%SZ",gmtime(&t));
		break;
	      }
	    case 't': // Unix time_t
		ss << tstart.tv_sec;
		break;
	    case 'V': // '--' if VLAN is present
		if(vlan!=NO_VLAN) ss << "--";
		break;
	    case 'v': // VLAN number if VLAN is present
		if(vlan!=NO_VLAN) ss << vlan;
		break;
	    case 'C': // 'c' if connection_count >0
		if(connection_count>0) ss << "c";
                used_cc = true;
		break;
	    case 'c': // connection_count if connection_count >0
		if(connection_count>0) ss << connection_count;
                used_cc = true;
		break;
	    case '#': // always output connection count
		ss << connection_count;
		break;
	    case '%': // Output a '%'
		ss << "%";
		break;
	    default:
		std::cerr << "Invalid filename_template: " << filename_template << "\n";
		std::cerr << "unknown character: " << filename_template.at(i+1) << "\n";
		exit(1);
	    }
	    if(buf[0]) ss << buf;
	}
    }
    if(used_cc==false){
        std::cerr << "filename template MUST include %c or %C\n";
        exit(1);
    }
    return ss.str();
}

/** Note: This has a test-and-set security problem */

std::string flow::new_filename()
{
    /* Loop connection count until we find a file that doesn't exist */
    for(uint32_t connection_count=0;;connection_count++){
        std::string nfn = filename(connection_count);
        if(access(nfn.c_str(),F_OK)!=0) return nfn;
    }
    return std::string("<<CANNOT CREATE FILE>>");               // error; no file
}
