/*
 * Ken Withee
 * CSEP 561: Network Systems, Spring 2017
 * Assignment 2 - Simple Router
 *
 * Implemented based on, sr_router.c by casado@stanford.edu.
 *
 * Description:
 *
 * This file contains all the functions that interact directly
 * with the routing table, as well as the main entry method
 * for routing.
 */

/* GLOBAL INCLUDES */
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

/* LOCAL INCLUDES */
#include "sr_if.h"
#include "sr_rt.h"
#include "sr_router.h"
#include "sr_protocol.h"
#include "sr_arpcache.h"
#include "sr_utils.h"

/* Define macro for calculating size of headers. */
#define ETH_HDR_SIZE (sizeof(struct sr_ethernet_hdr))
#define IP_HDR_SIZE (sizeof(struct sr_ip_hdr))
#define ARP_HDR_SIZE (sizeof(struct sr_arp_hdr))
#define ICMP_HDR_SIZE (sizeof(struct sr_icmp_hdr))
#define ICMP_ERROR_HDR_SIZE (sizeof(struct sr_icmp_t11_hdr))
#define SR_IF_SIZE (sizeof(struct sr_if))
#define UDP_HDR_SIZE 8


/*
 * Initialize the routing subsystem.
 */
void sr_init(struct sr_instance *sr)
{
    /* REQUIRES */
    assert(sr);

    /* Initialize cache and cache cleanup thread */
    sr_arpcache_init(&(sr->cache));

    pthread_attr_init(&(sr->attr));
    pthread_attr_setdetachstate(&(sr->attr), PTHREAD_CREATE_JOINABLE);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);

    pthread_t thread;
    pthread_create(&thread, &(sr->attr), sr_arpcache_timeout, sr);

} /* sr_init */


/*
 * This method is called each time the router receives a packet on the
 * interface.  The packet buffer, the packet length and the receiving
 * interface are passed in as parameters. The packet is complete with
 * ethernet headers.
 *
 * Note: Both the packet buffer and the character's memory are handled
 * by sr_vns_comm.c that means do NOT delete either.  Make a copy of the
 * packet instead if you intend to keep it around beyond the scope of
 * the method call.
 *
 */
void sr_handlepacket(struct sr_instance* sr,
         uint8_t *packet,
         unsigned int len,
         char *interface)
{
   /* REQUIRES */
   assert(sr);
   assert(packet);
   assert(interface);

   int check_ethernet_result = 0;

   /* Print some output that we received a packet. */
   printf("*** -> Received packet of length %d\n",len);

   /* Grab ethernet frame so we can inspect it. */
   struct sr_ethernet_hdr *ethernet_hdr = 0;
   ethernet_hdr = (struct sr_ethernet_hdr *)packet;

   check_ethernet_result = check_ethernet(ethernet_hdr->ether_dhost,
         sr->if_list);
   /* Is this ethernet frame for one of my interfaces? If not, drop packet. */
   if (check_ethernet_result == 1)
   {
      if (ntohs(ethernet_hdr->ether_type) == ethertype_arp)
      {
         deal_arp(sr, packet, len, interface);
      }
      else if (ntohs(ethernet_hdr->ether_type) == ethertype_ip)
      {
         deal_ip(sr, packet, len, interface);
      }
   }
} /* sr_handlepacket */


/*
 * Forward an IP packet.
 * When we get here we know the ETHERNET FRAME is for one
 * of our interfaces. We also know that the IP is NOT for
 * one of our interfaces so that means we need to forward
 * it to the next hop.
 */
void forward_ip_packet(struct sr_instance* sr,
         uint8_t *packet,
         unsigned int len,
         char *interface)

{
   /* REQUIRES */
   assert(sr);
   assert(packet);
   assert(interface);

   struct sr_arpentry *arpentry = 0;
   struct sr_arpreq *arpreq = 0;
   struct sr_ethernet_hdr *ethernet_hdr = 0;
   struct sr_ethernet_hdr *ethernet_hdr2 = 0;
   struct sr_ip_hdr *ip_hdr = 0;
   struct sr_if *outbound_if = 0;
   outbound_if = calloc(1, SR_IF_SIZE);
   int route_result = 0;
   uint8_t *tmppkt = 0;

   ip_hdr = (struct sr_ip_hdr *)(packet + ETH_HDR_SIZE);
   ethernet_hdr = (struct sr_ethernet_hdr *)packet;

   /* Decrement TTL. */
   ip_hdr->ip_ttl--;
   /* Redo checksum since we changed the TTL. */
   ip_hdr->ip_sum = 0x0;
   ip_hdr->ip_sum = cksum(ip_hdr, IP_HDR_SIZE);

   /* If the TTL expired, send a time exceeded ICMP error packet. */
   if (ip_hdr->ip_ttl == 0)
      send_icmp_error(sr, packet, len, interface, 11, 0);


   /* Look up longest match interface in routing table.*/
   route_result = check_route(sr, packet, len, outbound_if);
   if(route_result == 1)
   {
      /* Lookup the IP-MAC mapping in the cache. */
      arpentry = sr_arpcache_lookup((struct sr_arpcache *)&sr->cache, 
            ip_hdr->ip_dst);

      /* If the IP-MAC is in cache, send it. */
      if (arpentry)
      {
         /* Update the MAC addresses */
         memcpy(ethernet_hdr->ether_dhost, arpentry->mac, 6);
         /* the source mac is based on the outbound interface */
         memcpy(ethernet_hdr->ether_shost, outbound_if->addr, 6);

         /* The ethertype is the same, since we are just forwarding */
         sr_send_packet(sr, packet, len, (const char *)outbound_if->name);

         /* free arpentry that we got back */
         free(arpentry);
      }
      /* Else, it is not in cache, add it to req queue. */
      else
      {
         /* Make a copy of the packet. */
         tmppkt = malloc(len);
         memcpy(tmppkt, packet, len);

         ethernet_hdr2 = (struct sr_ethernet_hdr *)tmppkt;
         /* zero out destination MAC in Ethernet header */
         memset(ethernet_hdr2->ether_dhost, 0, sizeof(uint8_t)*6);
         /* set source mac to my mac for the outgoing interface */
         memcpy(ethernet_hdr2->ether_shost, outbound_if->addr, 6);

         /* Add request to queue. */
         arpreq = sr_arpcache_queuereq((struct sr_arpcache *)&sr->cache,
               ip_hdr->ip_dst, tmppkt, len, outbound_if->name);

         /* Handle this arp request. */
         handle_arpreq(sr, arpreq);

         /* Send ARP request for IP */
         /*send_arp_request(sr, packet, len, outbound_if->name);*/

         /* Free the packet we created. */
         free(tmppkt);
      }
   }

   /* Free the outbound_if memory. */
   free(outbound_if);
} /* forward_ip_packet */


/* 
 * Send an ARP request. 
 */
void send_arp_request(struct sr_instance *sr,
         uint8_t *packet,
         unsigned int len,
         char *if_name)
{
   /* REQUIRES */
   assert(sr);
   assert(packet);
   assert(if_name);

   uint8_t *buf;
   struct sr_ethernet_hdr *ethernet_hdr = 0;
   struct sr_arp_hdr *arp_hdr = 0;
   struct sr_if *byeface = 0;
   struct sr_ip_hdr *ip_hdr = 0;
   uint8_t breadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
   unsigned int length = 0; 

   byeface = malloc(SR_IF_SIZE);

   /* Set up. */
   ip_hdr = (struct sr_ip_hdr *)(packet + ETH_HDR_SIZE);
   byeface = sr_get_interface(sr, if_name);
   buf = malloc(ETH_HDR_SIZE + ARP_HDR_SIZE);
   ethernet_hdr = (struct sr_ethernet_hdr *)buf;
   arp_hdr = (struct sr_arp_hdr *)(buf + ETH_HDR_SIZE);
   
   /* ETHERNET HEADER TO SEND */
   memcpy(ethernet_hdr->ether_shost, byeface->addr, ETHER_ADDR_LEN);
   memcpy(ethernet_hdr->ether_dhost, breadcast, ETHER_ADDR_LEN);
   ethernet_hdr->ether_type = htons(ethertype_arp);

   /* ARP HEADER TO SEND */
   memcpy(arp_hdr->ar_sha, byeface->addr, ETHER_ADDR_LEN);
   memcpy(arp_hdr->ar_tha, breadcast, ETHER_ADDR_LEN);
   arp_hdr->ar_hrd = htons(arp_hrd_ethernet); 
   arp_hdr->ar_pro = htons(ethertype_ip);
   arp_hdr->ar_hln = ETHER_ADDR_LEN;
   arp_hdr->ar_pln = 4;
   arp_hdr->ar_op = htons(arp_op_request);
   arp_hdr->ar_sip = byeface->ip;
   arp_hdr->ar_tip = ip_hdr->ip_dst;

   length = ETH_HDR_SIZE + ARP_HDR_SIZE;
   sr_send_packet(sr, buf, length, byeface->name);
} /* send_arp_request() */


/*
 * Deal with an ARP packet.
 */
void deal_arp(struct sr_instance *sr,
         uint8_t *packet,
         unsigned int len,
         char *interface)
{
   /* REQUIRES */
   assert(sr);
   assert(packet);
   assert(interface);

   struct sr_arp_hdr *arp_hdr = 0;
   struct sr_ethernet_hdr *ethernet_hdr = 0;
   struct sr_arpentry *arpentry = 0;
   struct sr_arpreq *arpreq = 0;

   ethernet_hdr = (struct sr_ethernet_hdr *)packet;
   arp_hdr = (struct sr_arp_hdr *)(packet + ETH_HDR_SIZE);

   /* If this is an arp request to us, send reply. */
   if (ntohs(arp_hdr->ar_op) == arp_op_request)
   {
      reply_to_arp_request(sr, packet, len, interface);
   }
   /* Else it is a reply to one of our requests. */
   else if (ntohs(arp_hdr->ar_op) == arp_op_reply)
   {
      /* entry = arpcache_lookup(next_hop_ip) */
      arpentry = sr_arpcache_lookup(&sr->cache, arp_hdr->ar_sip);

      /* if arpentry, which means an IP-MAC mapping for this IP*/
      if (arpentry != 0)
      {
         /* Get outbound interface for IP */
         struct sr_if *out_if = 0;
         out_if = calloc(1, SR_IF_SIZE);
         check_route(sr, packet, len, out_if);

         /* send */
         sr_send_packet(sr, packet, len, out_if->name);
         free(arpentry);
      }
      /* else there isn't a mapping, insert it */
      else
      {
         arpreq = sr_arpcache_insert(&sr->cache, arp_hdr->ar_sha, 
               arp_hdr->ar_sip);

         /* if arpreq is not 0 there are requests in queue, send. */
         if (arpreq != 0)
         {
            struct sr_packet *current;
            struct sr_ethernet_hdr *tmpeth = 0;

            current = arpreq->packets;
            tmpeth = (struct sr_ethernet_hdr *)current->buf;

            memcpy(tmpeth->ether_dhost, ethernet_hdr->ether_shost, 6);

            sr_send_packet(sr, current->buf, current->len, current->iface);
         }
      }
   }
} /* deal arp */


/*
 * Update the ethernet headers and send the packet. 
 */
int update_eth_send_packet(struct sr_instance *sr,
         uint8_t *buf,
         unsigned int len,
         uint32_t destination_ip,
         struct sr_packet *packets,
         uint8_t dest_mac[])
{
   /* REQUIRES */
   assert(sr);
   assert(buf);
   assert(packets);

   struct sr_packet *current;
   struct sr_ethernet_hdr *ethernet_hdr = 0;

   current = packets;
   while (current != 0)
   {

      /* Encapsulate with correct dest mac.  */
      ethernet_hdr = (struct sr_ethernet_hdr *)current->buf;
      memcpy(ethernet_hdr->ether_shost, dest_mac, ETHER_ADDR_LEN);

      ethernet_hdr->ether_type = htons(0x0800);

      sr_send_packet(sr, current->buf, current->len, current->iface);

      current = current->next;
   }
   return 0;
} /* update_eth_send_packet */


/*
 * Deal with an IP packet.
 */
void deal_ip(struct sr_instance *sr,
         uint8_t *packet,
         unsigned int len,
         char *interface)
{
   /* REQUIRES */
   assert(sr);
   assert(packet);
   assert(interface);

   int validate_ip_result = 0;
   int check_interface_result = 0;
   uint32_t target_ip = 0;
   struct sr_if *dest_interface = 0;
   struct sr_ip_hdr *ip_hdr = 0;

   /* Get the target IP of this packet. */
   ip_hdr = (struct sr_ip_hdr *)(packet + ETH_HDR_SIZE);
   target_ip = ip_hdr->ip_dst;

   /* If packet is not valid then drop it. */
   validate_ip_result = validate_ip(sr, packet, len);
   if (validate_ip_result != 1)
      return;

   /* Check the IP packet for its destination. 1 means our interface, 0 not. */
   check_interface_result = check_interface(sr, target_ip, &dest_interface);

   /* If the packet is to one of our interfaces. */
   if (check_interface_result == 1)
   {
      /* If packet is an ICMP echo request, send an ICMP echo reply. */
      if (ip_hdr->ip_p == ip_protocol_icmp)
         reply_to_icmp_request(sr, packet, len, interface, dest_interface);
      /* If packet contains a TCP or UDP payload, send an ICMP port unreachable. */
      else if (ip_hdr->ip_p == ip_protocol_tcp || ip_hdr->ip_p == ip_protocol_udp)
         send_icmp_error(sr, packet, len, interface, 3, 3);
      /* Otherwise, ignore the packet. */
      else
         return;
   }
   /* Else, forward it */
   else
   {
      forward_ip_packet(sr, packet, len, interface);
   }
} /* deal_ip */


/*
 * Check if mac is meant for me (broadcast or one of my
 * interfaces. Return 1 if for me, 0 if not.
 */
int check_ethernet(uint8_t dest_mac[], struct sr_if *my_ifs)
{
   /* REQUIRES */
   assert(my_ifs);

   /* broadcast address */
   uint8_t breadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

   /* First check if it is a broadcast */
   if (compare_macs(dest_mac, breadcast))
      return 1;

   /* If it is not a broadcast, check if ethernet frame
    * is for one of the MAC's on my interfaces.
    */
   struct sr_if *walker = my_ifs;
   while (walker)
   {
      if (compare_macs(dest_mac, walker->addr))
         return 1;
      walker = walker->next;
   }

   return 0;
} /* check_ethernet */


/* 
 * Compare to mac's. Return 1 if match, 0 if not.
 */
int compare_macs(uint8_t mac1[], uint8_t mac2[])
{
   int i = 0;
   for (i = 0; i < 6; i++)
      if (mac1[i] != mac2[i])
         return 0;

   return 1;
} /* compare_macs */


/* 
 * Checks to see if a given IP is assigned to one of 
 * our interfaces, if so returns a pointer to the interface.
 * returns 1 for match or 0 for not equal 
 */
int check_interface(struct sr_instance *sr, uint32_t ip, 
      struct sr_if **dest_interface)
{
   /* REQUIRES */
   assert(sr);
   assert(dest_interface);

   /* get reference to interfaces */
   struct sr_if *walker = sr->if_list;
   while (walker)
   {
      if (walker->ip == ip)
      {
         *dest_interface = walker;
         return 1;
      }
      walker = walker->next;
   }

   return 0;
} /* check_interface */


/*
 * What is the next hop? 
 */
int check_route(struct sr_instance *sr,
         uint8_t *packet,
         unsigned int len,
         struct sr_if *outbound_if)
{
   /* REQUIRES */
   assert(sr);
   assert(packet);
   assert(outbound_if);

   struct sr_if *tmp_face = 0;
   struct sr_ip_hdr *ip_hdr = 0;
   ip_hdr = (struct sr_ip_hdr *)(packet + ETH_HDR_SIZE);

   /* Walk through the routing table and look for a match. */
   struct sr_rt *walker = sr->routing_table;
   while (walker)
   {
      if (ip_hdr->ip_dst == walker->dest.s_addr)
      {
         tmp_face = sr_get_interface(sr, walker->interface);
         memcpy(outbound_if->name, tmp_face->name, 32);
         memcpy(outbound_if->addr, tmp_face->addr, 6);
         return 1;
      }
      walker = walker->next;
   }

   /* Note that we have a default route that has a mask of 0.0.0.0 
    * which is everything. So we will always be able to send to the 
    * default gateway which is out our 10.0.1.1 interface. If we
    * didn't have a default route we would need to send an ICMP
    * net unreachable (type 3, code 0).
    *
    * Set default interface. 
    */
   tmp_face = sr_get_interface(sr, "eth3");
   memcpy(outbound_if->name, tmp_face->name, 32);
   memcpy(outbound_if->addr, tmp_face->addr, 6);
   return 1;
      
} /* check_route */


/*
 * Send an ICMP reply.
 */
void reply_to_icmp_request(struct sr_instance *sr,
      uint8_t *packet,
      unsigned int len,
      char *interface,
      struct sr_if *requested_interface)
{
   /* REQUIRES */
   assert(sr);
   assert(packet);
   assert(interface);
   assert(requested_interface);

   struct sr_ethernet_hdr *ether_frame = 0;
   struct sr_ip_hdr *ip_packet = 0;
   struct sr_icmp_hdr *icmp_datagram = 0;
   /*struct sr_if *fooface = 0;*/
 
   ether_frame = (struct sr_ethernet_hdr *)packet;
   memcpy(ether_frame->ether_dhost, ether_frame->ether_shost,
         ETHER_ADDR_LEN);
   memcpy(ether_frame->ether_shost, sr->if_list->addr, ETHER_ADDR_LEN);
   ether_frame->ether_type = htons(ethertype_ip);
 
   ip_packet = (struct sr_ip_hdr *)(packet + ETH_HDR_SIZE);
   /*fooface = sr_get_interface(sr, interface);*/
   ip_packet->ip_dst = ip_packet->ip_src;
   ip_packet->ip_src = requested_interface->ip;
 
   icmp_datagram = (struct sr_icmp_hdr *)(packet + ETH_HDR_SIZE + IP_HDR_SIZE);
   icmp_datagram->icmp_type = 0;

   /*CHECKSUMS */
   ip_packet->ip_sum = 0x0;
   ip_packet->ip_sum = cksum(ip_packet, IP_HDR_SIZE);
 
   sr_send_packet(sr, packet, len, interface);
}/* icmp_reply */


/*
 * Send an arp reply.
 */
void reply_to_arp_request(struct sr_instance* sr,
         uint8_t *packet,
         unsigned int len,
         char *interface)
{
   /* REQUIRES */
   assert(sr);
   assert(packet);
   assert(interface);

   struct sr_ethernet_hdr *ethernet_hdr = 0;
   struct sr_arp_hdr *arp_hdr = 0;
   ethernet_hdr = (struct sr_ethernet_hdr *)packet;
   arp_hdr = (struct sr_arp_hdr *)(packet + ETH_HDR_SIZE);

   /* Change senders mac and IP to destination mac and IP. */
   memcpy(ethernet_hdr->ether_dhost, ethernet_hdr->ether_shost, 6);
   memcpy(arp_hdr->ar_tha, ethernet_hdr->ether_shost, 6);
   uint32_t tmp_ip = arp_hdr->ar_tip;
   arp_hdr->ar_tip = arp_hdr->ar_sip;
   arp_hdr->ar_sip = tmp_ip;

   /* Set senders mac to mac of the interface it came in on. */
   struct sr_if *inbound_if = 0;
   inbound_if = sr_get_interface(sr, interface);
   memcpy(ethernet_hdr->ether_shost, inbound_if->addr, 6);
   memcpy(arp_hdr->ar_sha, inbound_if->addr, 6);

   /* Change the opcode to a reply. */
   arp_hdr->ar_op = htons(arp_op_reply);
   
   /* Send back. */
   sr_send_packet(sr, packet, len, interface);

} /* reply_to_arp_request */


/* 
 * Checks if a packet is valid. Large enough to hold IP and
 * has the correct checksum.
 * Return 1 for valid, 0 for invalid.
 */
int validate_ip(struct sr_instance* sr,
         uint8_t *packet,
         unsigned int len)
{
   /* REQUIRES */
   assert(sr);
   assert(packet);

   uint16_t tmp_sum = 0;
   struct sr_ip_hdr *ip_hdr = 0;
   ip_hdr = (struct sr_ip_hdr *)(packet + ETH_HDR_SIZE);

   /* Is the packet big enough? */
   if (ip_hdr->ip_len < IP_HDR_SIZE)
      return 0;

   /* Is the checksum correct? Make sure we don't change yet, since we
    * need the original checksum if responding to traceroute. */
   /* Grab original checksum. */
   tmp_sum = ip_hdr->ip_sum;
   /* Set checksum to 0 so we can calculate it again. */
   ip_hdr->ip_sum = 0x0;
   ip_hdr->ip_sum = cksum(ip_hdr, IP_HDR_SIZE);
   if (tmp_sum != ip_hdr->ip_sum)
      return 0;

   /* If we make it to here, packet is valid. */
   return 1;

} /* validate_ip */


/* 
 * Send an ICMP packet.
 *                               type  code
 * Echo Reply:                   0     0
 * Destination Net Unreachable:  3     0
 * Destination Host Unreachable: 3     1
 * Port Unreachable:             3     3
 * Time Exceeded:                11    0
 */
void send_icmp_error(struct sr_instance *sr,
         uint8_t *packet,
         unsigned int len,
         char *interface,
         uint8_t type,
         uint8_t code)
{
   /* REQUIRES */
   assert(sr);
   assert(packet);
   assert(interface);

   struct sr_ethernet_hdr *ethernet_hdr = 0;
   struct sr_ip_hdr *ip_hdr = 0;
   struct sr_ethernet_hdr *ethernet_hdr_out = 0;
   struct sr_ip_hdr *ip_hdr_out = 0;
   struct sr_icmp_t3_hdr *icmp_t3_hdr = 0;
   struct sr_if *outbound_if = 0;
   uint8_t *out_packet = 0;
   uint16_t total_len = 0;
   int i = 0;
   uint8_t *mem_walker = 0;

   /* Total length of packet. */
   total_len = ETH_HDR_SIZE + IP_HDR_SIZE + ICMP_ERROR_HDR_SIZE;
   
   /* Allocate memory for outbound packet. */
   out_packet = calloc(1, total_len);
   
   /* Get the outbound interface, same it came in on in this case. */
   outbound_if = sr_get_interface(sr, interface);

   /* inbound packet */
   ethernet_hdr = (struct sr_ethernet_hdr *)packet;
   ip_hdr = (struct sr_ip_hdr *)(packet + ETH_HDR_SIZE);
   /* outbound packet */
   ethernet_hdr_out = (struct sr_ethernet_hdr *)out_packet;
   ip_hdr_out = (struct sr_ip_hdr *)(out_packet + ETH_HDR_SIZE);
   icmp_t3_hdr = (struct sr_icmp_t3_hdr *)(out_packet 
         + ETH_HDR_SIZE + IP_HDR_SIZE);

   /* ETHERNET */
   memcpy(ethernet_hdr_out->ether_dhost, ethernet_hdr->ether_shost,
         ETHER_ADDR_LEN);
   memcpy(ethernet_hdr_out->ether_shost, outbound_if->addr, 
         ETHER_ADDR_LEN);
   ethernet_hdr_out->ether_type = ethernet_hdr->ether_type;


   /* ICMP */
   icmp_t3_hdr->icmp_type = type;
   icmp_t3_hdr->icmp_code = code;
   icmp_t3_hdr->icmp_sum = 0x0;
   icmp_t3_hdr->unused = 0x0;
   icmp_t3_hdr->next_mtu = 0x0;

   /* COPY ORIGINAL IP HEADER ( 20 bytes ) */
   mem_walker = (uint8_t *)ip_hdr;
   for (i=0; i<28; i++)
      icmp_t3_hdr->data[i] = *(mem_walker + i);

   /* calculate ICMP cksum */
   icmp_t3_hdr->icmp_sum = cksum(icmp_t3_hdr, ICMP_ERROR_HDR_SIZE);

   /* IP */
   ip_hdr->ip_ttl = 64;
   /*ip_hdr->ip_id += 8;*/
   ip_hdr->ip_p = ip_protocol_icmp;
   ip_hdr->ip_dst = ip_hdr->ip_src;
   ip_hdr->ip_src = outbound_if->ip;
   ip_hdr->ip_len = htons(IP_HDR_SIZE + ICMP_ERROR_HDR_SIZE);
   ip_hdr->ip_sum = 0x0;
   ip_hdr->ip_sum = cksum(ip_hdr, IP_HDR_SIZE);

   /* COPY IP HDR to outbound packet */
   memcpy(ip_hdr_out, ip_hdr, IP_HDR_SIZE);

   /* send the icmp error packet out */
   sr_send_packet(sr, out_packet, total_len, interface);

   /* free the packet we sent */
   free(out_packet);
}
