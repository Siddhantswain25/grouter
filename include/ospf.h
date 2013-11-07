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
#include "gnet.h"
#include "graph.h"
#include <stdint.h>
#include <endian.h>

#define OSPF_HELLO_MESSAGE       1      //Hello message
#define OSPF_LINK_STATUS_UPDATE  4      //Link status update = Link state advertisement
#define OSPF_HEADER_SIZE		 24		//24 bytes
#define OSPF_HELLO_MSG_SIZE		 20     //20 bytes, without neighbours IP addresses
#define OSPF_LSA_HEADER_SIZE	 20		//20 byte header
#define OSPF_LS_UPDATE_SIZE		 4		// 4 bytes in update excluding links
#define OSPF_LINK_SIZE			 16		// 16 bytes per link advertised
#define OSPF_NETMASK_ADDR        {0x00, 0xFF, 0xFF, 0xFF}
#define OSPF_ZERO_ADDR			 {0x00, 0x00, 0x00, 0x00}
/*
 * common OSPF (v2) header 
 */
typedef struct _ospfhdr_t
{
	uint8_t version;                  	// version always = 2
	uint8_t type;                     	// type either 1 (hello message) or 4 (Link state update-LSA)
	uint16_t pkt_len;					// total message length 
	uchar ip_src[4];					// src ip address of router sending message
	uint32_t area_id;             		// Area ID always = 0
	uint16_t cksum;                   	// checksum
	uint16_t authtype;                	// authentication type always = 0
	uchar auth[8];						// authentication always = null
} ospfhdr_t; 

 
/*
 * OSPF (v2) HELLO message 
 */
typedef struct _ospf_hello_t
{
	uchar netmask[4];						// network mask, always 255.255.255.0
	uint8_t options;                  		// options always = 0
	uint8_t priority;                     	// always = 0
	uint16_t hello_interval;				// always = 10 
	uint32_t dead_interval;             	// always = 40
	uchar designed_router_addr[4];			// always = 0
	uchar bkp_router_addr[4];				// always = 0
	uchar nbours_addr[MAX_INTERFACES][4];	//list of neighbours
} ospf_hello_t; 
 
/*
 * OSPF (v2) Link State Advertisement (LSA) message 
 */
typedef struct _ospf_lsa_hdr_t
{
	uint16_t age;							// always = 0
	uint16_t type;							// always = 1? not specified on requirements
	uchar link_state_id[4];   				// ip addr of router sending LSA
	uchar ads_router[4];					// ip addr of router sending LSA
	uint32_t seq_num;             			// seq num from 0 - inf, don't care about wrap around
	uint16_t cksum;                  		// chksum always = 0
	uint16_t ls_length;                     // length of lsa message
} ospf_lsa_hdr_t; 

typedef struct _ospf_link_t 
{
	uchar link_id[4];
	uchar link_data[4];
	uint16_t link_type;
	uint16_t empty;
	uint16_t empty2;
	uint16_t metric;

} ospf_link_t;

typedef struct _ospf_ls_update_t
{
	uint16_t word; 							//not on RFC.
	uint16_t num_links;						//numbers of links on this message
	ospf_link_t links[MAX_INTERFACES];		//list of neighbours
} ospf_ls_update_t;

/*
 * neighbour table entry
 */
typedef struct _nbour_entry_t 
{
	bool is_empty;                     	// indicate entry used or not
	bool bidirectional;                 // bidirectional value, 0=no, 1=yes
	bool is_stub;						// if ip address represents a stub network: 0 = no, i = yes
	uchar nbour_ip_addr[4];				// neighbour ip address
	uchar iface_ip_addr[4];				// interface ip address
	int interface_id;
	struct timeval tv;					//time value of last hello message received
} nbour_entry_t;


void OSPFInit();
void OSPFProcessPacket(gpacket_t *in_pkt);
void OSPFProcessHelloMessage(gpacket_t *in_pkt);
void OSPFProcessLSA(gpacket_t *in_pkt);
void OSPFSendHello();
void OSPFSendLSA();
void OSPFSendHelloThread();
void OSPFNeighbourLivenessChecker();
void craftCommonOSPFHeader(ospfhdr_t *ospfhdr, int ospf_pkt_size, int pkt_type);
void parseLinks(ospf_ls_update_t *update, Node *node);
void updateRoutingTable();
void OSPFprintTopology();

/*Neighbours management*/
void NeighboursTableInit();
int addNeighbourEntry(uchar *iface_ip_addr, uchar *nbour_ip_addr, int interface_id);
void addStubToGraph(int index);
void deleteNeighbourEntry(uchar *nbour_ip_addr);
int findNeighbourIndex(uchar *nbour_ip_addr);
int findAllNeighboursIPs(uchar buf[][4]);
int isNeighbourBidirectional(uchar *nbour_ip_addr);
int getEmptyIndex();
void printNeighboursTable();
int setBidirectionalFlag(uchar *nbour_ip_addr, bool flag);
int setStubToTrueFlag(uchar *nbour_ip_addr);
int isInterfaceDead(uchar *iface_ip_addr);

#endif
