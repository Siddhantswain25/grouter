/*
 * ospf.h (header file for OSPF protocol
 * AUTHOR: Lineker Tomazeli, Jake, Jeremie Bedard
 *         
 * DATE: Nov 8, 2013
 *
 */

#ifndef __OSPF_H_
#define __OSPF_H_

#include <sys/types.h>
#include "grouter.h"
#include "message.h"
#include <stdint.h>
#include <endian.h>

#define OSPF_HELLO_MESSAGE       1      //Hello message
#define OSPF_LINK_STATUS_UPDATE  4      //Link status update = Link state advertisement

typedef struct _ospfhdr_t
{
	uint8_t version;                  // version always = 2
	uint8_t type;                     // type either 1 (hello message) or 4 (Link state update-LSA)
	uint16_t pkt_len;                 // total message length 
	uchar ip_src[4];				// src ip address of router sending message
	uint32_t area_id;             	// Area ID always = 0
	uint16_t cksum;                   // checksum
	uint16_t authtype;                // authentication type always = 0
	uchar auth[8];					// authentication always = null
} ospfhdr_t; 

void OSPFInit();
void OSPFProcessPacket(gpacket_t *in_pkt);
void OSPFProcessHelloMessage(gpacket_t *in_pkt);
void OSPFProcessLSA(gpacket_t *in_pkt);
#endif