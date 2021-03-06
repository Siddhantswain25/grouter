/*
 * ospf.c (implementation file for the OSPF module)
 * AUTHOR: 
 * VERSION: Beta
 */
#include "protocols.h"
#include "ospf.h"
#include "ip.h"
#include "graph.h"
# include "routetable.h"
#include <slack/err.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>

//extern mtu_entry_t MTU_tbl[MAX_MTU];
nbour_entry_t nbours_tbl[MAX_INTERFACES];
Node *graph;
uint32_t mySeqNum = 0;

extern route_entry_t route_tbl[MAX_ROUTES];

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

void updateRoutingTable() {

	verbose(2,"---------- updateRoutingTable() ---------- \n");
	char tmpbuf[MAX_TMPBUF_LEN];
	NextHop *nh; //nh = head
	uchar interfaces[MAX_INTERFACES][4];
	int numInterfaces = getInterfaces(interfaces);
	nh = calculateDijkstra(graph, interfaces, numInterfaces);
	printNextHops(nh);
	printNextHopList(nh);

	NextHop *cur = nh;
	uchar zero_ip[] = OSPF_ZERO_ADDR;
	RouteTableInit(route_tbl);
	while (cur != NULL) {

		//check if next hop is diff than 0.0.0.0
		int index = -1;
		if ((COMPARE_IP(cur->nh_ip, zero_ip)) == 0) 
		{
			verbose(2, "IP is 0.0.0.0\n");
			//find neighbour using next_hop ip
			
			verbose(2, "HACK IP : %s\t", IP2Dot(tmpbuf,cur->rsubmask));
			index = findNeighbourIndex(cur->rsubmask);
		} else {
			verbose(2, "IP is NOT 0.0.0.0\n");
			//find neighbour using next_hop ip
			index = findNeighbourIndex(cur->nh_ip);
		}
		if (index >= 0) {
			verbose(2, "found Neighbour at [%d]\n", index );
			//get interface Id
			int iface_id = nbours_tbl[index].interface_id;
			verbose(2,"Adding Route for: \n");
			verbose(2, "Network Address\tSubnet Mask\tNext Hop\tiface_id\n");
			verbose(2, "%s\t", IP2Dot(tmpbuf, cur->rnetwork));
			verbose(2, "%s\t", IP2Dot(tmpbuf, cur->rsubmask));
			verbose(2, "%s\t", IP2Dot(tmpbuf, cur->nh_ip));
			verbose(2, "%d\n", iface_id);
			
			if(iface_id > 0) {
				verbose(2, "found Interface : %d\n", iface_id);
				//add route to fwd table
				Dot2IP("255.255.255.0", cur->rsubmask);
				addRouteEntry(route_tbl, cur->rnetwork, cur->rsubmask, cur->nh_ip, iface_id);
			} else {
				printf("Couldn't found a valid Interface. Are you sure this neighbour is valid and alive?\n", iface_id);
			}
			
		} else {
			printf("Couldn't find this next hop neighour : %s\n",IP2Dot(tmpbuf, cur->nh_ip));
		}
		
		cur = cur->next;
	}
	//How do we get the interface id?

	// TODO update forwarding table if necessary.
	/*
	 * Add a route entry to the table, if entry found update, else fill in an empty one,
	 * if no empty entry, overwrite a used one, indicated by rtbl_replace_indx
	 */
	//void addRouteEntry(route_entry_t route_tbl[], uchar* nwork, uchar* nmask, uchar* nhop, int interface)
	//void printRouteTable(route_entry_t route_tbl[])

	
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
	verbose(2,"[OSPFProcessHelloMessage]:: Received Hello Message. processing... \n");
	//printGPacket(in_pkt, 3, "IP_ROUTINE");
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
			gHtonl(tmpbuf, ospfhdr->ip_src), in_pkt->frame.src_interface);

	// We don't added the neighbours of our neighbours to our list
	// but we check to see if they have us on their list, if YES, we have a bidirectional connection
	for (i = 0; i < num_nbours; ++i) {
		int result;
		/*verbose(2, "COMPARE_IP( %s  , %s ) = %d \n",IP2Dot(tmpbuf, in_pkt->frame.src_ip_addr), 
		 IP2Dot(tmpbuf, hellomsg->nbours_addr[i]),
		 COMPARE_IP(in_pkt->frame.src_ip_addr,
		 hellomsg->nbours_addr[i]));*/
		if (COMPARE_IP(in_pkt->frame.src_ip_addr, hellomsg->nbours_addr[i])
				== 0) {
			
			//flip bidirectional flag
			int result = setBidirectionalFlag(gHtonl(tmpbuf, ospfhdr->ip_src),
					TRUE);
			if(result == 1) {
				verbose(2, "[OSPFProcessHelloMessage]:: New bidirectional connection\n");

				/* NOT completed. We should insert logic here if we start with an empty fwd table
				verbose(2, "Updating Routing tables with new/existing neighbour\n");
				uchar rnetwork[4];
				uchar submask[4];
				uchar next_hop[4];
				int iface_id = in_pkt->frame.src_interface; //interface id
				verbose(2, "InterfaceID = %d\n", iface_id);
				COPY_IP(rnetwork, gNtohl(tmpbuf+10,in_pkt->frame.src_ip_addr));
				//set last byte to 0
				rnetwork[3] = 0;
				//verbose(2, "NEW rnetwork : %s \n", IP2Dot(tmpbuf+5, in_pkt->frame.src_ip_addr));
				//verbose(2, "NEW rnetwork : %s \n", IP2Dot(tmpbuf+20, rnetwork));
				verbose(2, "NEW rnetwork : %s \n", IP2Dot(tmpbuf+20,  gNtohl(tmpbuf+100,rnetwork)));
				
				COPY_IP(submask, gNtohl(tmpbuf+30, hellomsg->netmask));
				verbose(2, "NEW submask : %s \n", IP2Dot(tmpbuf+50, gNtohl(tmpbuf,hellomsg->netmask)));
				
				COPY_IP(next_hop, gNtohl(tmpbuf+60,in_pkt->frame.nxth_ip_addr));
				verbose(2, "NEW next_hop : %s \n", IP2Dot(tmpbuf+70, gNtohl(tmpbuf,in_pkt->frame.nxth_ip_addr)));

				addRouteEntry(route_tbl, gNtohl(tmpbuf,rnetwork), gNtohl(tmpbuf+10,next_hop), iface_id);
				*/

				uchar *incomingInterface = in_pkt->frame.src_ip_addr;
				uchar *senderIP = ospfhdr->ip_src;
				// We can add this node to our graph.
				// Find the node in the graph corresponding to the interface we received on
				Node *found = getNodeByIP(graph, incomingInterface);
				if (found == NULL) {
					// Create a new node
					found = malloc(sizeof(Node));
					COPY_IP(found->ip, incomingInterface);
					found->seq_Numb = 0;
					found->list = NULL;

					graph = addNode(graph, found);
				}
				// Add this neighbor as a connection
				Link *newLink = malloc(sizeof(Link));
				COPY_IP(newLink->linkId, gNtohl(tmpbuf, senderIP));
				newLink->linkId[0] = IP_ZERO_PREFIX;
				COPY_IP(newLink->linkData, gNtohl(tmpbuf, senderIP));
				newLink->linkType = TYPE_ANY_TO_ANY;
				found->list = addLink(found->list, newLink);

				verbose(2, "[OSPFProcessHelloMessage]:: Added new neighbor to my graph:\n");
				verbose(2, "Interface: %s\n", IP2Dot(tmpbuf, found->ip));
				verbose(2, "Neighbor link ID: %s\n", IP2Dot(tmpbuf, newLink->linkId));
				verbose(2, "Neighbor link Data: %s\n", IP2Dot(tmpbuf, newLink->linkData));

				//bcast this change
				OSPFSendLSA();
				
			}
			
			
		}
	}
	//if nothing changes, do nothing
}

//TODO merge linked list stuff with graph in dijkstra if there's time.
void OSPFProcessLSA(gpacket_t *in_pkt) {
	
	verbose(2, "[OSPFProcessLSA]:: processing incoming LSA packet...");

	char src_ip_buf[MAX_TMPBUF_LEN];
	char tmpbuf[MAX_TMPBUF_LEN];
	ip_packet_t *ip_pkt = (ip_packet_t *) in_pkt->data.data;
	ospfhdr_t *ospfhdr = (ospfhdr_t *) ((uchar *) ip_pkt
			+ ip_pkt->ip_hdr_len * 4);
	ospf_lsa_hdr_t *lsahdr = (ospf_lsa_hdr_t *) ((uchar *) ospfhdr
			+ OSPF_HEADER_SIZE);
	ospf_ls_update_t *lsa = (ospf_ls_update_t *) ((uchar *) lsahdr
			+ OSPF_LSA_HEADER_SIZE);
	int seqNo = lsahdr->seq_num;
	uchar *src_ip = gNtohl(src_ip_buf, lsahdr->ads_router);

	verbose(2, "[OSPFProcessLSA]:: processing incoming LSA packet from %s with sequence no. %d\n",
			IP2Dot(tmpbuf, src_ip), seqNo);

	// Drop packets that are from me
	if (!hasInterface(src_ip)) {

		// Check if we've seen this advertisement.
		Node *found = getNodeByIP(graph, src_ip);
		if (found == NULL || found->seq_Numb < seqNo) {
			// We haven't seen this LSA yet
			verbose(2, "[OSPFProcessLSA]:: Haven't seen this LSA before\n");
			if (found == NULL) {
				verbose(2, "[OSPFProcessLSA]:: Haven't received from this sender yet\n");
				found = malloc(sizeof(Node));
				COPY_IP(found->ip, src_ip);
				found->seq_Numb = seqNo;
				found->list = NULL;
				graph = addNode(graph, found);
				//printGraph(graph);
			}
			else {
				// We've received from this sender, but this is a new LSA.
				verbose(2, "[OSPFProcessLSA]:: Have received from this sender before, updating seqNum\n");
				found->seq_Numb = seqNo;

				parseLinks(lsa, found);
				//printGraph(graph);
			}


			parseLinks(lsa, found);

			// Broadcast the packet
			verbose(2, "Broadcasting this packet\n");

			IPOutgoingBcastAllInterPkt(in_pkt, ip_pkt->ip_pkt_len, 0, OSPF_PROTOCOL);

			//run dijkstra and update fwd tables
			updateRoutingTable();
		}
		else {
			verbose(2, "Received duplicate packet, not broadcasting!\n");
		}
	}
	else {
		verbose(2, "I am the sender, ignoring this packet\n");
	}
}

/* Parse the links from update and add them as links in node. */
void parseLinks(ospf_ls_update_t *update, Node *node) {
	uchar tmpbuf[MAX_TMPBUF_LEN];

	// Delete the previous links
	Link *head = node->list;
	Link *tmp;
	while (head != NULL) {
		tmp = head;
		head = tmp->next;
		free(tmp);
	}
	node->list = NULL;

	// Add the new links
	int i;
	for (i = 0; i < update->num_links; i++) {
		verbose(2, "[parseLinks]:: ADDING A LINK!\n");
		ospf_link_t *curLink = &(update->links[i]);
		Link *newLink = malloc(sizeof(Link));
		COPY_IP(newLink->linkId, gNtohl(tmpbuf, curLink->link_id));
		COPY_IP(newLink->linkData, gNtohl(tmpbuf+20, curLink->link_data));
		newLink->linkType = curLink->link_type;

		node->list = addLink(node->list, newLink);
	}
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
	verbose(2, "[OSPFSendLSA]:: Broadcasting LSA Message");

	gpacket_t *out_pkt = (gpacket_t *) malloc(sizeof(gpacket_t));
	ip_packet_t *ipkt = (ip_packet_t *) (out_pkt->data.data);
	ipkt->ip_hdr_len = 5; // size of IP header with NO options!!
	ospfhdr_t *ospfhdr = (ospfhdr_t *) ((uchar *) ipkt + ipkt->ip_hdr_len * 4); //jumping ptr to end of ip header

	// Get all neighbors
	nbour_entry_t my_nbours[MAX_INTERFACES];
	int num_of_neighbours, i = 0;
	num_of_neighbours = findAllNeighbours(my_nbours);

	// set LSA message payload total length (lenght in LSA header)
	int lsa_length = OSPF_LS_UPDATE_SIZE + num_of_neighbours * OSPF_LINK_SIZE;
	//set OSPF message payload total length (length in OSPF header)
	int total_length = OSPF_LSA_HEADER_SIZE + lsa_length;

	/*LSA Header*/

	ospf_lsa_hdr_t *lsa_hdr = (ospf_lsa_hdr_t *) ((uchar *) ospfhdr
			+ OSPF_HEADER_SIZE);
	lsa_hdr->age = 0;
	lsa_hdr->type = 1;
	//lsa_hdr->link_state_id and ads_router set when broadcasting. For now all zeroes:
	for (i = 0; i < 4; i++) {
		lsa_hdr->link_state_id[i] = 0;
		lsa_hdr->ads_router[i] = 0;
	}
	lsa_hdr->seq_num = mySeqNum;
	mySeqNum++;
	lsa_hdr->cksum = 0;
	lsa_hdr->ls_length = lsa_length;

	/*LSA links*/

	ospf_ls_update_t *lsupdate = (ospf_ls_update_t *) ((uchar *) lsa_hdr
			+ OSPF_LSA_HEADER_SIZE);

	lsupdate->word = 0; //always 0, not specified on RFC
	lsupdate->num_links = num_of_neighbours;

	uchar link_id_ip[4];

	for (i = 0; i < num_of_neighbours; i++) {
		char tmpbuf[MAX_TMPBUF_LEN];
		ospf_link_t *curLink = &(lsupdate->links[i]);

		COPY_IP(link_id_ip, my_nbours[i].nbour_ip_addr);
		link_id_ip[0] = IP_ZERO_PREFIX;

		verbose(3, "[OSPFSendLSA]:: adding neighbour : %s \n",
				IP2Dot(tmpbuf, link_id_ip));

		// link_id is the network of this link
		COPY_IP(curLink->link_id, gHtonl(tmpbuf+20, link_id_ip));
		// link_data is the netmask for stubs and router IP for any-to-any
		// TODO handle stub case
		COPY_IP(curLink->link_data,
				gHtonl(tmpbuf+40, my_nbours[i].nbour_ip_addr));
		curLink->metric = 1;
		curLink->empty = 0;
		curLink->empty2 = 0;

		curLink->link_type = (my_nbours[i].is_stub) ? 3 : 2;
	}

	//builds ospf header and calculates chksum
	verbose(3, "[OSPFSendLSA]:: calling craftCommonOSPFHeader");
	craftCommonOSPFHeader(ospfhdr, total_length, OSPF_LINK_STATUS_UPDATE);

	verbose(3, "[OSPFSendLSA]:: sending to broadcast function");
	IPOutgoingBcastAllInterPkt(out_pkt, total_length + OSPF_HEADER_SIZE, 1,
			OSPF_PROTOCOL);
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
		//verbose(2, "SLEEPED FOR %6.3f ms \n", elapsed_time);
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
			if (nbours_tbl[i].is_empty == FALSE && !nbours_tbl[i].is_stub) {
				gettimeofday(&second, NULL);
				elapsed_time = subTimeVal(&second, &nbours_tbl[i].tv);
				verbose(3, "[OSPFNeighbourLivenessChecker]:: Received hello %6.3f ms ago from %s\n",
						elapsed_time,
						IP2Dot(tmpbuf, nbours_tbl[i].nbour_ip_addr));
				//COPY_IP(buf[count], nbours_tbl[i].nbour_ip_addr);
				if (elapsed_time > 40000) {
					// Remove from graph
					Node *interface = getNodeByIP(graph, nbours_tbl[i].iface_ip_addr);
					verbose(2, "Trying to remove %s from graph\n", IP2Dot(tmpbuf, nbours_tbl[i].nbour_ip_addr));
					interface->list = removeLinkByLinkData(interface->list, nbours_tbl[i].nbour_ip_addr);

					// Remove node if no links lefts
					if (interface->list == NULL) {
						removeNodeByIP(graph, nbours_tbl[i].iface_ip_addr);
					}

					// Remove from neighbour table
					deleteNeighbourEntry(nbours_tbl[i].nbour_ip_addr);
					verbose(3, "[OSPFNeighbourLivenessChecker]:: Neighbour %s is dead. deleting...",
							IP2Dot(tmpbuf, nbours_tbl[i].nbour_ip_addr));

					//delete routing entries where neibour_ip is the nexthop
					deleteRouteEntryByInterface(route_tbl, nbours_tbl[i].interface_id);

					updateRoutingTable();
					//delete routing entry where subnet = neibour_ip ending in 0 (DONT NEED)

					verbose(2, "[OSPFNeighbourLivenessChecker]:: LSA bcast bc a neighbour was removed\n");
					//bcast this change
					OSPFSendLSA();
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

	uchar netmask[] = OSPF_NETMASK_ADDR; //for debug
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
	int num_of_neighbours, i=0;
	num_of_neighbours = findAllNeighboursIPs (list_nbours);

	if (num_of_neighbours > 0) {
		for (i = 0; i < num_of_neighbours; i++) {
			//verbose(2, "[OSPFSendHello]:: adding neighbour : %s \n", IP2Dot(tmpbuf, list_nbours[i]));
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

/* return 1 if flag was flipped. 0 otherwise*/
int setBidirectionalFlag(uchar *nbour_ip_addr, bool flag) {
	int index;
	char tmpbuf[MAX_TMPBUF_LEN];
	verbose(2,
			"[setBidirectionalFlag]:: Flipping bidirectional flag for : %s \n",
			IP2Dot(tmpbuf, nbour_ip_addr));
	index = findNeighbourIndex(nbour_ip_addr);
	if (index >= 0) {
		if(nbours_tbl[index].bidirectional != flag) {
			nbours_tbl[index].bidirectional = flag;
			return 1; //we return 1, bc changed is good
		}
	}
	return 0;
}

/*
 * set bidirectional flag to true if entry for that neighbour ip address already exist in the neighbour table
 * return 1 if already exist, return 0 if no entry exists
 */
int setStubToTrueFlag(uchar *nbour_ip_addr) {
	int index;
	char tmpbuf[MAX_TMPBUF_LEN];
	verbose(2, "[setStubFlag]:: Flipping Stub flag for : %s \n",
			IP2Dot(tmpbuf, nbour_ip_addr));
	index = findNeighbourIndex(nbour_ip_addr);
	if (index >= 0) {
		nbours_tbl[index].is_stub = 1;
		return 1; //we return 1, bc changed is good
	}
	return 0;
}

/*
 * add neighbour entry, update bidirectional property if we already had it entry.
 * return 1 if already exist
 */
int addNeighbourEntry(uchar *iface_ip_addr, uchar *nbour_ip_addr, int interface_id) {
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
		nbours_tbl[index].interface_id = interface_id;
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

/* Given the index of the stub neighbor, adds it to the graph (adds a link to the node representing the interface) */
void addStubToGraph(int index) {
	nbour_entry_t *nbour = &(nbours_tbl[index]);
	Node *found = getNodeByIP(graph, nbour->iface_ip_addr);
	if (found == NULL) {
		found = malloc(sizeof(Node));
		found->seq_Numb = 0;
		COPY_IP(found->ip, nbour->iface_ip_addr);
		found->list = NULL;

		graph = addNode(graph, found);
	}
	Link *newLink = malloc(sizeof(Link));
	COPY_IP(newLink->linkId, nbour->nbour_ip_addr);
	uchar netmask[] = IP_BCAST_ADDR;
	netmask[0] = IP_ZERO_PREFIX;
	COPY_IP(newLink->linkData, netmask);
	newLink->linkType = TYPE_STUB;
	found->list = addLink(found->list, newLink);
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

int isInterfaceDead(uchar *iface_ip_addr) {
	char tmpbuf[MAX_TMPBUF_LEN];
	verbose(3, "inside findNeighbourByInterface \n");
	int i;
	for (i = 0; i < MAX_INTERFACES; i++) {
		if (nbours_tbl[i].is_empty == TRUE
			&& COMPARE_IP(nbours_tbl[i].iface_ip_addr, iface_ip_addr)
						== 0) {
			verbose(3, "[%d] found dead neighour, Interface %s is Dead \n", i, IP2Dot(tmpbuf, nbours_tbl[i].iface_ip_addr));
			return 1;
		}
	}

	return 0;
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
			buf[count] = nbours_tbl[i];
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
	return -1;
}

/*
 * print neighbours table
 */
void printNeighboursTable() {
	int i;
	char tmpbuf[MAX_TMPBUF_LEN];
	printf( "------------------------------------------------------------------\n");
	printf( " O S P F :  N E I G H B O U R S  T A B L E  \n");
	printf( "------------------------------------------------------------------\n");
	printf( "index\tIface\t\tIfaceID\tNeighbour\tisStub\tisBidirectional \n");
	for (i = 0; i < MAX_INTERFACES; i++) {
		if (nbours_tbl[i].is_empty == FALSE) {
			printf( "%d\t", i);
			printf( "%s\t", IP2Dot(tmpbuf, nbours_tbl[i].iface_ip_addr));
			printf( "%d\t", nbours_tbl[i].interface_id);
			if (strlen(tmpbuf) <= 8)
				printf( "\t"); //just to pretty print
			printf( "%s\t", IP2Dot(tmpbuf, nbours_tbl[i].nbour_ip_addr));
			printf( "%d\t", nbours_tbl[i].is_stub);
			printf( "%d\n", nbours_tbl[i].bidirectional);
			
		}
	}
	verbose(2, "-----------------------------------------------------------------\n");
	return;
}

void OSPFprintTopology() {
	printGraph(graph);
}

