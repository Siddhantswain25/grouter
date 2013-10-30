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
#include <slack/err.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>



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
	uchar dst_ip[4];
	uchar *temp_dst_ip = "192.168.2.255"; //for debug
	Dot2IP(temp_dst_ip, dst_ip);
	
	
	//set authentication  - uchar auth[4];
	//ospfhdr->auth = NULL;

	//set total pkt_len
	int pkt_size = 24;
	ospfhdr->pkt_len = pkt_size; 
	
	IPOutgoingPacket(out_pkt, dst_ip, pkt_size, 1, OSPF_PROTOCOL);
}