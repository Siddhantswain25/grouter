/*
 * ospf.c (implementation file for the OSPF module)
 * AUTHOR: 
 * VERSION: Beta
 */
#include "protocols.h"
#include "ospf.h"
#include "ip.h"
#include <slack/err.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>

//extern mtu_entry_t MTU_tbl[MAX_MTU];
nbour_entry_t nbours_tbl[MAX_INTERFACES];
seq_num_tbl_t *seq_num_tbl;
uint32_t mySeqNum = 0;

void OSPFInit() {
	//RouteTableInit(route_tbl);
	NeighboursTableInit(nbours_tbl);

	pthread_t threadid;
	pthread_t threadid2;

	int threadstat = pthread_create((pthread_t *) &threadid, NULL,
			(void *) OSPFSendHelloThread, (void *) NULL);
	if (threadstat != 0) {
		verbose(1, "[OSPFInit]:: unable to create Hello Message thread ...");
		//return EXIT_FAILURE;
	}

	threadstat = pthread_create((pthread_t *) &threadid2, NULL,
			(void *) OSPFNeighbourLivenessChecker, (void *) NULL);
	if (threadstat != 0) {
		verbose(1, "[OSPFInit]:: unable to create Hello Message thread ...");
		//return EXIT_FAILURE;
	}
}

void OSPFProcessPacket(gpacket_t *in_pkt) {
	ip_packet_t *ip_pkt = (ip_packet_t *) in_pkt->data.data;
	int iphdrlen = ip_pkt->ip_hdr_len * 4; //move ptr to begin of he ospf header
	ospfhdr_t *ospfhdr = (ospfhdr_t *) ((uchar *) ip_pkt + iphdrlen);

	switch (ospfhdr->type) {
	case OSPF_HELLO_MESSAGE:
		verbose(2, "[OSPFProcessPacket]:: OSPF processing for HELLO MESSAGE");
		OSPFProcessHelloMessage(in_pkt);
		break;

	case OSPF_LINK_STATUS_UPDATE:
		verbose(2,
				"[OSPFProcessPacket]:: OSPF processing for Link State Advertisement message");
		OSPFProcessLSA(in_pkt);
		break;
	}
}

void OSPFProcessHelloMessage(gpacket_t *in_pkt) {
	verbose(2,
			"[OSPFProcessHelloMessage]:: Received Hello Message. processing... \n");
	char tmpbuf[MAX_TMPBUF_LEN];
	ip_packet_t *ip_pkt = (ip_packet_t *) in_pkt->data.data;
	ospfhdr_t *ospfhdr = (ospfhdr_t *) ((uchar *) ip_pkt
			+ ip_pkt->ip_hdr_len * 4);
	ospf_hello_t *hellomsg = (ospf_hello_t *) ((uchar *) ospfhdr
			+ OSPF_HEADER_SIZE);
	int i, num_nbours;
	//calculate # of nbours
	num_nbours = (ospfhdr->pkt_len - OSPF_HEADER_SIZE - OSPF_HELLO_MSG_SIZE)
			/ 4; //divide by 4, since each IP has 4 bytes
	int exist = 0;

	exist = addNeighbourEntry(in_pkt->frame.src_ip_addr,
			gHtonl(tmpbuf, ospfhdr->ip_src));
	if (exist > 0) {
		//reset timer
		verbose(2, "NOT IMPLEMENTED: RESET TIMER \n");
	} else {
		//create new timer
		verbose(2, "NOT IMPLEMENTED: CREATE NEW TIMER \n");
	}

	// We don't added the neighbours of our neighbours to our list
	// but we check to see if they have us on their list, if YES, we have a bidirectional connection
	for (i = 0; i < num_nbours; ++i) {
		int result;
		/*printf("COMPARE_IP( %s  , %s ) = %d \n",IP2Dot(tmpbuf, in_pkt->frame.src_ip_addr), 
		 IP2Dot(tmpbuf, hellomsg->nbours_addr[i]),
		 COMPARE_IP(in_pkt->frame.src_ip_addr,
		 hellomsg->nbours_addr[i]));*/
		if (COMPARE_IP(in_pkt->frame.src_ip_addr, hellomsg->nbours_addr[i])
				== 0) {
			//flip bidirectional flag
			int result = setBidirectionalFlag(gHtonl(tmpbuf, ospfhdr->ip_src),
					TRUE);
			//TODO: send to next step
			//OSPFSendLSA();
		}
	}
	//if nothing changes, do nothing
}

void OSPFProcessLSA(gpacket_t *in_pkt) {
	verbose(2, "[OSPFProcessLSA]:: processing incoming LSA packet...");

	char src_ip_buf[MAX_TMPBUF_LEN];
	char tmpbuf2[MAX_TMPBUF_LEN];
	ip_packet_t *ip_pkt = (ip_packet_t *) in_pkt->data.data;
	ospfhdr_t *ospfhdr = (ospfhdr_t *) ((uchar *) ip_pkt
			+ ip_pkt->ip_hdr_len * 4);
	ospf_lsa_hdr_t *lsahdr = (ospf_lsa_hdr_t *) ((uchar *) ospfhdr + OSPF_HEADER_SIZE);
	ospf_ls_update_t *lsa = (ospf_ls_update_t *) ((uchar *) lsahdr + OSPF_LSA_HEADER_SIZE);
	int seqNo = lsahdr->seq_num;
	uchar *src_ip = gNtohl(src_ip_buf, ip_pkt->ip_src);

	verbose(2, "[OSPFProcessLSA]:: processing incoming LSA packet from %s with sequence no. %d", IP2Dot(tmpbuf2, src_ip), seqNo);

	//TODO remove me!!
	printOSPFPacket(in_pkt);

	// Check if we've seen this advertisement.
	seq_num_entry_t *cur = seq_num_tbl->head;
	while(cur != NULL) {
		if (COMPARE_IP(cur->src_ip, src_ip)) break;
		cur = cur->next;
	}
	if (cur == NULL) {
		// Add this entry to table
		verbose(2, "[OSPFProcessLSA]:: First time seeing LSA from this IP");
		seq_num_entry_t *newentry = (seq_num_entry_t *) malloc(sizeof(seq_num_entry_t));
		newentry->seq_num = seqNo;
		COPY_IP(newentry->src_ip, src_ip);

		if (seq_num_tbl->head == NULL) {
			// List is empty
			seq_num_tbl->head = newentry;
			seq_num_tbl->tail = newentry;
		} else {
			seq_num_tbl->tail->next = newentry;
			seq_num_tbl->tail = seq_num_tbl->tail->next;
		}
	}
	else if (cur->seq_num < seqNo) {
		cur->seq_num = seqNo;
		verbose(2, "[OSPFProcessLSA]:: Not implemented: run Dijkstra, update fwd table");
		IPOutgoingBcastAllInterPkt(in_pkt, ip_pkt->ip_pkt_len, 1, OSPF_PROTOCOL);

		// TODO parse links, update graph & run dijkstra & update forwarding table if necessary.
	}

	// Else, already seen this message, do nothing (i.e. drop packet).

}

void craftCommonOSPFHeader(ospfhdr_t *ospfhdr, int ospf_pkt_size, int pkt_type) {
	/* craft OSPF Header */
	ospfhdr->version = 2; //always 2
	ospfhdr->area_id = 0; //always 0
	ospfhdr->cksum = 0; //always 0.
	ospfhdr->authtype = 0; // always 0
	//ospfhdr->auth always NULL
	ospfhdr->pkt_len = ospf_pkt_size;
	ospfhdr->type = pkt_type;
}

void OSPFSendLSA() {
	printf("[OSPFSendLSA]:: Not Implemented -> broadcast LSA");

	verbose(2, "[OSPFSendLSA]:: Broadcasting LSA Message");

	char tmpbuf[MAX_TMPBUF_LEN];

	gpacket_t *out_pkt = (gpacket_t *) malloc(sizeof(gpacket_t));
	ip_packet_t *ipkt = (ip_packet_t *) (out_pkt->data.data);
	ipkt->ip_hdr_len = 5; // size of IP header with NO options!!
	ospfhdr_t *ospfhdr = (ospfhdr_t *) ((uchar *) ipkt + ipkt->ip_hdr_len * 4); //jumping ptr to end of ip header

	/* craft LSA Message */

	/*LSA Header*/
	ospf_lsa_hdr_t *lsa_hdr = (ospf_lsa_hdr_t *) ((uchar *) ospfhdr
			+ OSPF_HEADER_SIZE);
	lsa_hdr->age = 0;
	lsa_hdr->type = 1;
	//lsa_hdr->link_state_id and ads_router set on next step
	lsa_hdr->seq_num = mySeqNum;
	mySeqNum++;
	lsa_hdr->cksum = 0;

	/*LSA links*/

	nbour_entry_t my_nbours[MAX_INTERFACES];
	int num_of_neighbours, i = 0;
	num_of_neighbours = findAllNeighbours(my_nbours); //get all neighbours



	//set lsa message total length
	int ls_length = OSPF_LSA_HEADER_SIZE + OSPF_LS_UPDATE_SIZE
			+ (OSPF_LINK_SIZE * num_of_neighbours);
	ospf_ls_update_t *lsupdate = (ospf_ls_update_t *) ((uchar *) lsa_hdr
			+ OSPF_LSA_HEADER_SIZE);

	lsupdate->word = 0; //always 0, not specified on RFC
	lsupdate->num_links = num_of_neighbours;

	uchar link_id_ip[4];
	//adding any-to-any links
	for (i = 0; i < num_of_neighbours; i++) {
		lsupdate->links[i] = (ospf_link_t *) malloc(sizeof(ospf_link_t));

		printf("[OSPFSendLSA]:: adding neighbour : %s \n",
				IP2Dot(tmpbuf, my_nbours[i].nbour_ip_addr));
		COPY_IP(link_id_ip, my_nbours[i].nbour_ip_addr);
		link_id_ip[0] = IP_ZERO_PREFIX;
		COPY_IP(lsupdate->links[i]->link_id, gHtonl(tmpbuf, link_id_ip));
		//lsupdate->link_id //network address = neighbour IP with last byte = 0, eg.192.168.2.0

		//get interface router ip address
		//lsupdate->link_data //router IP sending LSA
		COPY_IP(lsupdate->links[i]->link_data, my_nbours[i].iface_ip_addr);
		lsupdate->links[i]->metric = 1;
		lsupdate->links[i]->empty = 0;
		lsupdate->links[i]->empty2 = 0;

		// TODO handle stub networks.
		lsupdate->links[i]->link_type = 2; //these are all any-to-any = 2

		//memcpy(lsupdate->links[i], link, sizeof(ospf_link_t)); //NOT SURE IF THIS IS WORKING
	}

//	//set total pkt_len on OSPF common header
//	int total_pkt_size = OSPF_HEADER_SIZE + OSPF_LSA_HEADER_SIZE + ls_length; //each nbour ip is 4 bytes * #of nbours

	//builds ospf header and calculates chksum
	craftCommonOSPFHeader(ospfhdr, ls_length, OSPF_LINK_STATUS_UPDATE);

	IPOutgoingBcastAllInterPkt(out_pkt, ls_length, 1, OSPF_PROTOCOL);

}

void OSPFSendHelloThread() {
	//struct timeval first;
	//struct timeval second;
	//double elapsed_time;
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	while (1) {
		OSPFSendHello();
		//gettimeofday(&first, NULL);
		usleep(10000000); //10seconds
		//gettimeofday(&second, NULL);
		//elapsed_time = subTimeVal(&second, &first);
		//printf("SLEEPED FOR %6.3f ms \n", elapsed_time);
	}
}

void OSPFNeighbourLivenessChecker() {
	char tmpbuf[MAX_TMPBUF_LEN];
	struct timeval second;
	double elapsed_time;
	int i;
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	while (1) {
		for (i = 0; i < MAX_INTERFACES; i++) {
			if (nbours_tbl[i].is_empty == FALSE) {
				gettimeofday(&second, NULL);
				elapsed_time = subTimeVal(&second, &nbours_tbl[i].tv);
				verbose(3, "Received hello %6.3f ms ago from %s\n",
						elapsed_time,
						IP2Dot(tmpbuf, nbours_tbl[i].nbour_ip_addr));
				//COPY_IP(buf[count], nbours_tbl[i].nbour_ip_addr);
				if (elapsed_time > 40000) {
					deleteNeighbourEntry(nbours_tbl[i].nbour_ip_addr);
					verbose(3, "Neighbour %s is dead. deleting...",
							IP2Dot(tmpbuf, nbours_tbl[i].nbour_ip_addr));

					//TODO: SEND LSA
				}

			}
		}

		usleep(5000000); //5seconds
	}
}

void OSPFSendHello() {
	verbose(2, "[OSPFSendHello]:: Broadcasting Hello Message");

	char tmpbuf[MAX_TMPBUF_LEN];

	gpacket_t *out_pkt = (gpacket_t *) malloc(sizeof(gpacket_t));
	ip_packet_t *ipkt = (ip_packet_t *) (out_pkt->data.data);
	ipkt->ip_hdr_len = 5; // size of IP header with NO options!!
	ospfhdr_t *ospfhdr = (ospfhdr_t *) ((uchar *) ipkt + ipkt->ip_hdr_len * 4); //jumping ptr to end of ip header

	/* craft HELLO Message */
	//ospfhdr->type = OSPF_HELLO_MESSAGE; //set type
	ospf_hello_t *hellomsg = (ospf_hello_t *) ((uchar *) ospfhdr
			+ OSPF_HEADER_SIZE);

	uchar netmask[] =
			OSPF_NETMASK_ADDR; //for debug
					COPY_IP(hellomsg->netmask, gHtonl(tmpbuf, netmask));
					hellomsg->options = 0;// options always = 0
					hellomsg->priority = 0;// always = 0
					hellomsg->hello_interval = 10;// always = 10
					hellomsg->dead_interval = 40;// always = 40

					//set designed router addr for 0.0.0.0
					uchar designed_router_addr[] = OSPF_ZERO_ADDR;//for debug
					COPY_IP(hellomsg->designed_router_addr, gHtonl(tmpbuf, designed_router_addr));

					//set backup router addr for 0.0.0.0
					uchar bkp_router_addr[] = OSPF_ZERO_ADDR;
					COPY_IP(hellomsg->bkp_router_addr, gHtonl(tmpbuf, bkp_router_addr));

					//added list of neighbours to packet
					uchar list_nbours[MAX_INTERFACES][4];
					int num_of_neighbours, i
=0;num_of_neighbours =
findAllNeighboursIPs (
	list_nbours
)	;

	if (num_of_neighbours > 0) {
		for (i = 0; i < num_of_neighbours; i++) {
			//printf("[OSPFSendHello]:: adding neighbour : %s \n", IP2Dot(tmpbuf, list_nbours[i]));
			COPY_IP(hellomsg->nbours_addr[i], list_nbours[i]);
		}
	}
	//set total pkt_len on OSPF common header
	int total_pkt_size = OSPF_HEADER_SIZE + OSPF_HELLO_MSG_SIZE
			+ (num_of_neighbours * 4); //each nbour ip is 4 bytes * #of nbours

	//builds ospf header and calculates chksum
	craftCommonOSPFHeader(ospfhdr, total_pkt_size, OSPF_HELLO_MESSAGE);

	IPOutgoingBcastAllInterPkt(out_pkt, total_pkt_size, 1, OSPF_PROTOCOL);
}

/*
 * initialize the MTU table to be empty
 */
void NeighboursTableInit() {
	int i;

	for (i = 0; i < MAX_INTERFACES; i++) {
		nbours_tbl[i].is_empty = TRUE;
	}
	verbose(2, "[NeighboursTableInit]:: table initialized..\n");
	return;
}

int setBidirectionalFlag(uchar *nbour_ip_addr, bool flag) {
	int index;
	char tmpbuf[MAX_TMPBUF_LEN];
	verbose(2,
			"[setBidirectionalFlag]:: Flipping bidirectional flag for : %s \n",
			IP2Dot(tmpbuf, nbour_ip_addr));
	index = findNeighbourIndex(nbour_ip_addr);
	if (index >= 0) {
		nbours_tbl[index].bidirectional = flag;
		return 1; //we return 1, bc changed is good
	}
	return 0;
}

/*
 * add neighbour entry, update bidirectional property if we already had it entry.
 * return 1 if already exist
 */
int addNeighbourEntry(uchar *iface_ip_addr, uchar *nbour_ip_addr) {
	char tmpbuf[MAX_TMPBUF_LEN];
	// check validity of the specified value, set to DEFAULT_MTU if invalid
	/*if (iface_ip_addr == NULL || nbour_ip_addr == NULL)
	 {
	 verbose(2, "[addNeighbourEntry]:: Either interface or neighbour IP are invalid \n");
	 }*/
	int index, retval;
	index = findNeighbourIndex(nbour_ip_addr);
	if (index >= 0) {
		verbose(2, "[addNeighbourEntry]:: neighbour %s already exist \n",
				IP2Dot(tmpbuf, nbour_ip_addr));
		retval = 1; //we return 1, bc already exist, nothing changes
	} else {
		index = getEmptyIndex(nbours_tbl);
		nbours_tbl[index].is_empty = FALSE;
		if (iface_ip_addr != NULL) {
			COPY_IP(nbours_tbl[index].iface_ip_addr, iface_ip_addr);
		}
		COPY_IP(nbours_tbl[index].nbour_ip_addr, nbour_ip_addr);
		verbose(2, "[addNeighbourEntry]:: added neighbour: %s \n",
				IP2Dot(tmpbuf, nbours_tbl[index].nbour_ip_addr));
		retval = 0;
	}
	//set or update timestamp of message received
	gettimeofday(&nbours_tbl[index].tv, NULL);
	return retval;
}

/*
 * delete the MTU entry that corresponds to the given index from
 * the MTU table.
 */

void deleteNeighbourEntry(uchar *nbour_ip_addr) {
	char tmpbuf[MAX_TMPBUF_LEN];
	int i;
	i = findNeighbourIndex(nbour_ip_addr);
	if (i >= 0) {
		nbours_tbl[i].is_empty = TRUE;
		verbose(2,
				"[deleteNeighbourEntry]:: Deleted reference to neighbour: %s\n",
				IP2Dot(tmpbuf, nbours_tbl[i].nbour_ip_addr));
		return;
	}
	verbose(2, "[deleteNeighbourEntry]:: Can't find entry for neighbour: %s\n",
			IP2Dot(tmpbuf, nbours_tbl[i].nbour_ip_addr));
	return;
}

/*
 * Returns index of the neighour found, -1 otherwise
 */
int findNeighbourIndex(uchar *nbour_ip_addr) {
	char tmpbuf[MAX_TMPBUF_LEN];

	int i;
	for (i = 0; i < MAX_INTERFACES; i++) {
		if (nbours_tbl[i].is_empty == FALSE
				&& COMPARE_IP(nbours_tbl[i].nbour_ip_addr, nbour_ip_addr)
						== 0) {
			return i;
		}
	}

	return -1;
}

int findAllNeighboursIPs(uchar buf[][4]) {
	int i, count = 0;
	char tmpbuf[MAX_TMPBUF_LEN];

	for (i = 0; i < MAX_INTERFACES; i++) {
		if (nbours_tbl[i].is_empty == FALSE) {
			COPY_IP(buf[count], nbours_tbl[i].nbour_ip_addr);
			count++;
		}
	}

	verbose(2, "[findAllNeighboursIPs]:: output buffer with %d IPs", count);
	return count;
}

int findAllNeighbours(nbour_entry_t buf[]) {
	int i, count = 0;
	char tmpbuf[MAX_TMPBUF_LEN];

	for (i = 0; i < MAX_INTERFACES; i++) {
		if (nbours_tbl[i].is_empty == FALSE) {
			//memcpy(buf[count], nbours_tbl[i], sizeof(nbour_entry_t)); THIS IS NOT WORKING!!!!
			count++;
		}
	}

	verbose(2, "[findAllNeighbours]:: output buffer with %d neighbours", count);
	return count;
}

int isNeighbourBidirectional(uchar *nbour_ip_addr) {
	int index;
	char tmpbuf[MAX_TMPBUF_LEN];
	index = findNeighbourIndex(nbour_ip_addr);
	if (index >= 0) {
		if (nbours_tbl[index].bidirectional == TRUE) {
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
void printNeighboursTable() {
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
			if (strlen(tmpbuf) <= 8)
				printf("\t"); //just to pretty print
			printf("%s\t", IP2Dot(tmpbuf, nbours_tbl[i].nbour_ip_addr));
			printf("%d\n", nbours_tbl[i].bidirectional);
		}
	}
	printf("---------------------------------------------------------\n");
	return;
}
