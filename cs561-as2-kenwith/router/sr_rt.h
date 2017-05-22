/*
 * Ken Withee
 * CSEP 561: Network Systems, Spring 2017
 * Assignment 2 - Simple Router
 *
 * Implemented based on work by casado@stanford.edu.
 *
 * Description:
 *
 * Methods and datastructures for handeling the routing table
 *
 */

#ifndef sr_RT_H
#define sr_RT_H

#ifdef _DARWIN_
#include <sys/types.h>
#endif

#include <netinet/in.h>

#include "sr_if.h"

/*
 * Struct:
 *
 */

struct sr_rt
{
    struct in_addr dest;
    struct in_addr gw;
    struct in_addr mask;
    char   interface[sr_IFACE_NAMELEN];
    struct sr_rt* next;
};


int sr_load_rt(struct sr_instance*,const char*);
void sr_add_rt_entry(struct sr_instance*, struct in_addr,struct in_addr,
                  struct in_addr, char*);
void sr_print_routing_table(struct sr_instance* sr);
void sr_print_routing_entry(struct sr_rt* entry);


#endif  /* --  sr_RT_H -- */