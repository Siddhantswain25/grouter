
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "grouter.h"
#include "message.h"
#include "protocols.h"
#include "ip.h"
#include "arp.h"
#include "ospf.h"


gpacket_t *duplicatePacket(gpacket_t *inpkt)
{
	gpacket_t *cpptr = (gpacket_t *) malloc(sizeof(gpacket_t));

	if (cpptr == NULL)
	{
		error("[duplicatePacket]:: error allocating memory for duplication.. ");
		return NULL;
	}
	memcpy(cpptr, inpkt, sizeof(gpacket_t));
	return cpptr;
}



void printSepLine(char *start, char *end, int count, char sep)
{
	int i;

	printf("%s", start);
	for (i = 0; i < count; i++)
		printf("%c", sep);
	printf("%s", end);
}


void printGPktFrame(gpacket_t *msg, char *routine)
{
	char tmpbuf[MAX_TMPBUF_LEN];

	printf("\n    P A C K E T  F R A M E  S E C T I O N of GPACKET @ %s \n", routine);
	printf(" SRC interface : \t %d\n", msg->frame.src_interface);
	printf(" SRC IP addr : \t %s\n", IP2Dot(tmpbuf, msg->frame.src_ip_addr));
	printf(" SRC HW addr : \t %s\n", MAC2Colon(tmpbuf, msg->frame.src_hw_addr));
	printf(" DST interface : \t %d\n", msg->frame.dst_interface);
	printf(" NEXT HOP addr : \t %s\n", IP2Dot(tmpbuf, msg->frame.nxth_ip_addr));
}


void printGPacket(gpacket_t *msg, int level, char *routine)
{
	printSepLine("", "\n", 70, '=');
	printGPktFrame(msg, routine);

	if (level >= 3)
		printGPktPayload(msg, level);

	printSepLine("\n", "\n", 70, '=');
}


void printGPktPayload(gpacket_t *msg, int level)
{
	int prot;

	prot = printEthernetHeader(msg);
	switch (prot)
	{
	case IP_PROTOCOL:
		prot = printIPPacket(msg);
		switch (prot)
		{
		case ICMP_PROTOCOL:
			printICMPPacket(msg);
			break;
		case UDP_PROTOCOL:
			printUDPPacket(msg);
		case TCP_PROTOCOL:
			printTCPPacket(msg);
		case OSPF_PROTOCOL:
			printOSPFPacket(msg);
		}
		break;
	case ARP_PROTOCOL:
		printARPPacket(msg);
		break;
	default:
		// ignore other cases for now!
		break;
	}
}

void printOSPFPacket(gpacket_t *msg)
{
	ip_packet_t *ipkt;
	ospfhdr_t *ospfhdr;
	char tmpbuf[MAX_TMPBUF_LEN];

	ipkt = (ip_packet_t *)msg->data.data;
	ospfhdr = (ospfhdr_t *)((uchar *)ipkt + ipkt->ip_hdr_len*4); 
	printf("OSPF: ----- OSPF Header -----\n");
	printf("OSPF: Version        		: %d\n", ospfhdr->version);
	printf("OSPF: Type           		: %d\n", ospfhdr->type);
	printf("OSPF: Total Length   		: %d Bytes\n", ospfhdr->pkt_len);
	printf("OSPF: Source         		: %s\n", IP2Dot(tmpbuf, gNtohl((tmpbuf+20), ospfhdr->ip_src)));
	printf("OSPF: Area ID        		: %d\n", ospfhdr->area_id);
	printf("OSPF: Checksum       		: 0x%X\n", ntohs(ospfhdr->cksum));
	printf("OSPF: Auth Type      		: %d\n", ospfhdr->authtype);
	printf("OSPF: Authentication 		: %s\n", ospfhdr->auth);

	if(ospfhdr->type == OSPF_HELLO_MESSAGE) {
		printOSPFHelloPacket(msg);
	}
	else if (ospfhdr->type == OSPF_LINK_STATUS_UPDATE) {
		printLSAHeader(msg);
		printLSUpdate(msg);
	}
}

void printLSAHeader(gpacket_t *msg) {
	char tmpbuf[MAX_TMPBUF_LEN];
	ip_packet_t *ipkt = (ip_packet_t *)msg->data.data;
	ospfhdr_t *ospfhdr = (ospfhdr_t *)((uchar *)ipkt + ipkt->ip_hdr_len*4);
	ospf_lsa_hdr_t *lsahdr = (ospf_lsa_hdr_t *) ((uchar *)ospfhdr + OSPF_HEADER_SIZE);
	printf("OSPF: ----- OSPF LSA Header -----\n");
	printf("OSPF: Age   				: %d\n", lsahdr->age);
	printf("OSPF: Type        			: %d\n", lsahdr->type);
	printf("OSPF: Link State ID         : %s\n", IP2Dot(tmpbuf, gNtohl(tmpbuf+20, lsahdr->link_state_id)));
	printf("OSPF: Advertising Router   	: %s\n", IP2Dot(tmpbuf, gNtohl(tmpbuf+20, lsahdr->ads_router)));
	printf("OSPF: Sequence Number       : %d\n", lsahdr->seq_num);
	printf("OSPF: Checksum     			: %d\n", lsahdr->cksum);
	printf("OSPF: Length      			: %d\n", lsahdr->ls_length);
}

void printLSUpdate(gpacket_t *msg) {
	char tmpbuf[MAX_TMPBUF_LEN];
	ip_packet_t *ipkt = (ip_packet_t *)msg->data.data;
	ospfhdr_t *ospfhdr = (ospfhdr_t *)((uchar *)ipkt + ipkt->ip_hdr_len*4);
	ospf_lsa_hdr_t *lsahdr = (ospf_lsa_hdr_t *) ((uchar *)ospfhdr + OSPF_HEADER_SIZE);
	ospf_ls_update_t *lsupdate = (ospf_ls_update_t *) ((uchar *)lsahdr + OSPF_LSA_HEADER_SIZE);

	printf("OSPF: ----- OSPF LS Update -----\n");
	printf("OSPF: Word   				: %d\n", lsupdate->word);
	printf("OSPF: Number of Links		: %d\n", lsupdate->num_links);
	printf("OSPF: Links					:\n");

	int i;
	for (i = 0; i < lsupdate->num_links; i++) {
		ospf_link_t link = lsupdate->links[i];
		printf("\n\tLink ID				: %s\n", IP2Dot(tmpbuf, gNtohl(tmpbuf+20, link.link_id)));
		printf("\tLink Data				: %s\n", IP2Dot(tmpbuf, gNtohl(tmpbuf+20, link.link_data)));
		printf("\tLink Type				: %d\n", link.link_type);
		printf("\tLink Metric			: %d\n", link.metric);
	}
}

void printOSPFHelloPacket(gpacket_t *msg) {
	ip_packet_t *ipkt;
	ospfhdr_t *ospfhdr;
	ospf_hello_t *hellomsg;
	char tmpbuf[MAX_TMPBUF_LEN];
	int num_nbours, i;
	ipkt = (ip_packet_t *)msg->data.data;
	ospfhdr = (ospfhdr_t *)((uchar *)ipkt + ipkt->ip_hdr_len*4);
	hellomsg = (ospf_hello_t *)((uchar *)ospfhdr + 24); //move ptr
	num_nbours = (ospfhdr->pkt_len - OSPF_HEADER_SIZE - OSPF_HELLO_MSG_SIZE)/4;
	
	printf("OSPF: ----- OSPF Hello Message -----\n");
	printf("OSPF: Network Mask   		: %s\n", IP2Dot(tmpbuf, gNtohl((tmpbuf), hellomsg->netmask)));
	printf("OSPF: Options        		: %d\n", hellomsg->options);
	printf("OSPF: Priority           	: %d\n", hellomsg->priority);
	printf("OSPF: Hello Interval   		: %d\n", hellomsg->hello_interval);
	printf("OSPF: Dead Interval         	: %d\n", hellomsg->dead_interval);
	printf("OSPF: Designed Router IP     	: %s\n", IP2Dot(tmpbuf, gNtohl((tmpbuf+20), hellomsg->designed_router_addr)));
	printf("OSPF: Backcup Router IP      	: %s\n", IP2Dot(tmpbuf, gNtohl((tmpbuf+20), hellomsg->bkp_router_addr)));
	printf("OSPF: Neighbours      		: ");
	
	//uchar *temp = hellomsg->nbours_addr;
	for (i = 0; i < num_nbours; ++i)
	{
		if(i > 0) printf("\t\t\t\t  ");
		printf("%s\n", IP2Dot(tmpbuf,hellomsg->nbours_addr[i]));
	}
}

int printEthernetHeader(gpacket_t *msg)
{
	char tmpbuf[MAX_TMPBUF_LEN];
	int prot;

	printf("\n    P A C K E T  D A T A  S E C T I O N of GMESSAGE \n");
	printf(" DST MAC addr : \t %s\n", MAC2Colon(tmpbuf, msg->data.header.dst));
	printf(" SRC MAC addr : \t %s\n", MAC2Colon(tmpbuf, msg->data.header.src));
	prot = ntohs(msg->data.header.prot);
	printf(" Protocol : \t %x\n", prot);

	return prot;
}


int printIPPacket(gpacket_t *msg)
{
	ip_packet_t *ip_pkt;
	char tmpbuf[MAX_TMPBUF_LEN];
	int tos;

	ip_pkt = (ip_packet_t *)msg->data.data;
	printf("IP: ----- IP Header -----\n");
	printf("IP: Version        : %d\n", ip_pkt->ip_version);
	printf("IP: Header Length  : %d Bytes\n", ip_pkt->ip_hdr_len*4);
	printf("IP: Total Length   : %d Bytes\n", ntohs(ip_pkt->ip_pkt_len));
	printf("IP: Type of Service: 0x%02X\n", ip_pkt->ip_tos);
	printf("IP:      xxx. .... = 0x%02X (Precedence)\n", IPTOS_PREC(ip_pkt->ip_tos));
	tos = IPTOS_TOS(ip_pkt->ip_tos);
	if (tos ==  IPTOS_LOWDELAY)
		printf("IP:      ...1 .... = Minimize Delay\n");
	else
		printf("IP:      ...0 .... = Normal Delay\n");
	if (tos == IPTOS_THROUGHPUT)
		printf("IP:      .... 1... = Maximize Throughput\n");
	else
		printf("IP:      .... 0... = Normal Throughput\n");
	if (tos == IPTOS_RELIABILITY)
		printf("IP:      .... .1.. = Maximize Reliability\n");
	else
		printf("IP:      .... .0.. = Normal Reliability\n");
	if (tos == IPTOS_MINCOST)
		printf("IP:      .... ..1. = Minimize Cost\n");
	else
		printf("IP:      .... ..0. = Normal Cost\n");
	printf("IP: Identification : %d\n", ntohs(ip_pkt->ip_identifier));
	printf("IP: Flags          : 0x%02X\n", ((ntohs(ip_pkt->ip_frag_off) & ~IP_OFFMASK)>>13));
	if ((ntohs(ip_pkt->ip_frag_off) & IP_DF) == IP_DF)
		printf("IP:      .1.. .... = do not fragment\n");
	else
		printf("IP:      .0.. .... = can fragment\n");
	if ((ntohs(ip_pkt->ip_frag_off) & IP_MF) == IP_MF)
		printf("IP:      ..1. .... = more fragment\n");
	else
		printf("IP:      ..0. .... = last fragment\n");
	printf("IP: Fragment Offset: %d Bytes\n", (ntohs(ip_pkt->ip_frag_off) & IP_OFFMASK));
	printf("IP: Time to Live   : %d sec/hops\n", ip_pkt->ip_ttl);

	printf("IP: Protocol       : %d\n", ip_pkt->ip_prot);
	printf("IP: Checksum       : 0x%X\n", ntohs(ip_pkt->ip_cksum));
	printf("IP: Source         : %s\n", IP2Dot(tmpbuf, gNtohl((tmpbuf+20), ip_pkt->ip_src)));
	printf("IP: Destination    : %s\n", IP2Dot(tmpbuf, gNtohl((tmpbuf+20), ip_pkt->ip_dst)));

	return ip_pkt->ip_prot;
}


void printARPPacket(gpacket_t *msg)
{
	arp_packet_t *apkt;
	char tmpbuf[MAX_TMPBUF_LEN];

	apkt = (arp_packet_t *) msg->data.data;

	printf(" ARP hardware addr type %x \n", ntohs(apkt->hw_addr_type));
	printf(" ARP protocol %x \n", ntohs(apkt->arp_prot));
	printf(" ARP hardware addr len %d \n", apkt->hw_addr_len);
	printf(" ARP protocol len %d \n", apkt->arp_prot_len);
	printf(" ARP opcode %x \n", ntohs(apkt->arp_opcode));
	printf(" ARP src hw addr %s \n", MAC2Colon(tmpbuf, apkt->src_hw_addr));
	printf(" ARP src ip addr %s \n", IP2Dot(tmpbuf, gNtohl((uchar *)tmpbuf, apkt->src_ip_addr)));
	printf(" ARP dst hw addr %s \n", MAC2Colon(tmpbuf, apkt->dst_hw_addr));
	printf(" ARP dst ip addr %s \n", IP2Dot(tmpbuf, gNtohl((uchar *)tmpbuf, apkt->dst_ip_addr)));
}


void printICMPPacket(gpacket_t *msg)
{

	printf("\n ICMP PACKET display NOT YET IMPLEMENTED !! \n");
}


void printUDPPacket(gpacket_t *msg)
{

	printf("\n UDP PACKET display NOT YET IMPLEMENTED !! \n");
}


void printTCPPacket(gpacket_t *msg)
{

	printf("\n TCP PACKET display NOT YET IMPLEMENTED !! \n");
}
