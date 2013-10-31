/*
 * ocpf.c (implementation file for the OSPF module)
 * AUTHOR: 
 * VERSION: Beta
 */
#include "protocols.h"
#include "ospf.h"
#include "ip.h"
#include "message.h"
#include "grouter.h"
#include "gnet.h"
#include <slack/err.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>

//extern mtu_entry_t MTU_tbl[MAX_MTU];
nbour_entry_t nbours_tbl[MAX_INTERFACES];

void OSPFInit()
{
	//RouteTableInit(route_tbl);
	NeighboursTableInit(nbours_tbl);

}

void OSPFProcessPacket(gpacket_t *in_pkt)
{
	printGPacket(in_pkt, 3, "IP_ROUTINE"); //for debug
	ip_packet_t *ip_pkt = (ip_packet_t *)in_pkt->data.data;
	int iphdrlen = ip_pkt->ip_hdr_len *4; //not sure about this 4!
	ospfhdr_t *ospfhdr = (ospfhdr_t *)((uchar *)ip_pkt + iphdrlen);

	switch (ospfhdr->type)
	{
	case OSPF_HELLO_MESSAGE:
		verbose(2, "[OSPFProcessPacket]:: OSPF processing for HELLO MESSAGE");
		OSPFProcessHelloMessage(in_pkt);
		break;

	case OSPF_LINK_STATUS_UPDATE:
		verbose(2, "[OSPFProcessPacket]:: OSPF processing for Link State Advertisement message");
		OSPFProcessLSA(in_pkt);
		break;
	}
}


void OSPFProcessHelloMessage(gpacket_t *in_pkt)
{

}

void OSPFProcessLSA(gpacket_t *in_pkt)
{

}



void OSPFSendHello()
{
	verbose(2, "[OSPFSendHello]:: Broadcasting Hello Message");

	//loop through each interface
	//findAllInterfaceIPs

	gpacket_t *out_pkt = (gpacket_t *) malloc(sizeof(gpacket_t));
	ip_packet_t *ipkt = (ip_packet_t *)(out_pkt->data.data);
	ipkt->ip_hdr_len = 5;                                  // no IP header options!!
	ospfhdr_t *ospfhdr = (ospfhdr_t *)((uchar *)ipkt + ipkt->ip_hdr_len*4); //jumping ptr to end of ip header
	uint16_t cksum;
	uchar *dataptr; //ptr to data 

	ospfhdr->type = OSPF_HELLO_MESSAGE; //set type
	ospfhdr->version = 2; //always 2
	ospfhdr->area_id = 0; //always 0
	ospfhdr->cksum = 0; //TODO: compute checksum as IP does.
	ospfhdr->authtype = 0; // always 0

	//set source IP - uchar ip_src[4];
	/*
	uchar *temp = "192.168.2.128"; //for debug
	uchar src_ip[4];
	Dot2IP(temp,src_ip);
	COPY_IP(ospfhdr->ip_src, src_ip);
	char tmpbuf[MAX_TMPBUF_LEN];
	printf("OSPF: Source###  : %s\n", IP2Dot(tmpbuf, gNtohl((tmpbuf+20), ospfhdr->ip_src)));
	*/
	//get destination IP
	//uchar dst_ip[4];
	//uchar *temp_dst_ip = "192.168.2.255"; //for debug
	//Dot2IP(temp_dst_ip, dst_ip);
	
	
	//set authentication  - uchar auth[4];
	//ospfhdr->auth = NULL;

	//set total pkt_len
	int pkt_size = 24;
	ospfhdr->pkt_len = pkt_size; 
	
	IPOutgoingBcastAllInterPkt(out_pkt, pkt_size, 1, OSPF_PROTOCOL);
}

/*
 * initialize the MTU table to be empty
 */
void NeighboursTableInit()
{
	int i;

	for(i = 0; i < MAX_INTERFACES; i++){
		nbours_tbl[i].is_empty = TRUE;
	}
	verbose(2, "[NeighboursTableInit]:: table initialized..\n");
	return;
}

/*
 * add neighbour entry, update bidirectional property if we already had it entry.
 */
void addNeighbourEntry(uchar *iface_ip_addr, uchar *nbour_ip_addr)
{
	char tmpbuf[MAX_TMPBUF_LEN];
	// check validity of the specified value, set to DEFAULT_MTU if invalid
	if (iface_ip_addr == NULL || nbour_ip_addr == NULL)
	{
		verbose(2, "[addNeighbourEntry]:: Either interface or neighbour IP are invalid \n");
	}
	int index; 

	if(index = findNeighbourIndex(nbour_ip_addr) >= 0) {
		nbours_tbl[index].bidirectional = TRUE;
		verbose(2, "[addNeighbourEntry]:: Flipping bidirectional flag for : %s \n",IP2Dot(tmpbuf, nbour_ip_addr));
	} else {
		index = getEmptyIndex(nbours_tbl);
		nbours_tbl[index].is_empty = FALSE;
		COPY_IP(nbours_tbl[index].iface_ip_addr, iface_ip_addr);
		COPY_IP(nbours_tbl[index].nbour_ip_addr, nbour_ip_addr);
		verbose(2,"[addNeighbourEntry]:: added neighbour: %s \n", IP2Dot(tmpbuf, nbours_tbl[index].nbour_ip_addr));
	}

	return;
}

/*
 * delete the MTU entry that corresponds to the given index from
 * the MTU table.
 */

void deleteNeighbourEntry(uchar *nbour_ip_addr)
{
	char tmpbuf[MAX_TMPBUF_LEN];
	int i;
	i = findNeighbourIndex(nbour_ip_addr);
	if(i >= 0)
	{
		nbours_tbl[i].is_empty = TRUE;
		verbose(2, "[deleteNeighbourEntry]:: Deleted reference to neighbour: %s\n", IP2Dot(tmpbuf, nbours_tbl[i].nbour_ip_addr));
		return;
	}
	verbose(2, "[deleteNeighbourEntry]:: Can't find entry for neighbour: %s\n", IP2Dot(tmpbuf, nbours_tbl[i].nbour_ip_addr));
	return;
}

/*
 * Returns index of the neighour found, -1 otherwise
 */
int findNeighbourIndex(uchar *nbour_ip_addr)
{
	char tmpbuf[MAX_TMPBUF_LEN];
	
	int i;
	for (i = 0; i < MAX_INTERFACES; i++) {
		if(nbours_tbl[i].is_empty == FALSE &&
		   COMPARE_IP(nbours_tbl[i].nbour_ip_addr, nbour_ip_addr) == 0)
		{
			return i;
		}
	}

	return -1;
}

int isNeighbourBidirectional(uchar *nbour_ip_addr) {
	int index;
	char tmpbuf[MAX_TMPBUF_LEN];
	index = findNeighbourIndex(nbour_ip_addr);
	if( index >= 0)
	{
		if(nbours_tbl[index].bidirectional == TRUE) {
			return TRUE;
		}
	}
	return FALSE;
}

int getEmptyIndex() {
	int i;
	for (i = 0; i < MAX_INTERFACES; i++) {
		if (nbours_tbl[i].is_empty == TRUE) {
			return i;
		}
	}
}

/*
 * print neighbours table
 */
void printNeighboursTableTable()
{
	int i;
	char tmpbuf[MAX_TMPBUF_LEN];
	printf("---------------------------------------------------------\n");
	printf(" O S P F :  N E I G H B O U R S  T A B L E  \n");
	printf("---------------------------------------------------------\n");
	printf("index\tInterface\tNeighbour\tisBidirectional \n");
	for (i = 0; i < MAX_INTERFACES; i++) {
		if (nbours_tbl[i].is_empty == FALSE) {
			printf("%d\t", i);
			printf("%s\t", IP2Dot(tmpbuf, nbours_tbl[i].iface_ip_addr));
			printf(" %s\t", IP2Dot(tmpbuf, nbours_tbl[i].nbour_ip_addr));
			printf("%d\n",nbours_tbl[i].bidirectional);
		}
	}
	printf("---------------------------------------------------------\n");
	return;
}