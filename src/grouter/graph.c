/*
 * Operations on the topology graph
 */
#include "graph.h"

void printuchar(uchar ip[]){
	verbose(2, "%d.%d.%d.%d", ip[3], ip[2], ip[1],ip[0]);
}


char* getucharstr(uchar ip[]){
	char* str = (char*)malloc(sizeof(char)*16);
	sprintf(str, "%d.%d.%d.%d", ip[3], ip[2], ip[1], ip[0]);

	///verbose(2, "%s", str);

	return str;	
}

//-------FUNCTIONS------------
//Given the node's ip(ip) and sequence number(seq_numb), returns a freshly new created baby node 
Node* createNode(char *ip, int seq_Numb){
	Node *newNode = (Node *)malloc(sizeof(Node));
	newNode->seq_Numb = seq_Numb;

	Dot2IP(ip, newNode->ip);

	return newNode;
}

//Given link ID (linkId), link data and link type, create freshly new created link and returns it
//If link type != 2 || 3 --> return NULL
Link* createLink(char *linkId, char *linkData, int linkType){
	//If linkType != 2 || 3 ===> return NULL
	if(!((linkType ==2) || (linkType ==3)))
		return NULL;

	Link *newLink = (Link *)malloc(sizeof(Link));

	newLink->linkType = linkType;
	Dot2IP(linkId, newLink->linkId);
	Dot2IP(linkData, newLink->linkData);

	return newLink;
}

//Returns the length of a graph given the head of the graph
int graphLength(Node *root){
	int length = 0;
	Node *node = root;
	while(node != NULL){
		//verbose(2, "Element found: %s\n",list->router->ip);
		node = node->next;
		length++;
	}
	//verbose(2, "Length of list is of : %d\n\n", length);
	return length;
}

//Returns the length of the neighbour list of a Node given a node
int neighbourListLength(Node *node){
	int length = 0;
	Link *link = node->list;
	while(link != NULL){
		link = link->next;
		length++;
	}

	return length;
}

//Add router node rToAdd to router addToThisR's list
void addNextNode(Node **root, Node *node){
	node->next = 	(*root);
	*root = node;
}

//Add router rToAdd to router addToThisR's list
void addNeighbour(Node *node, Link *link){
	link->next = node->list;
	node->list = link;
}

//Remove all neighbours from
void removeAllNeighbours(Node *node){
	node->list = NULL;
}

//Given the head of a graph, prints all elements contained in list
void printGraphJeremie(Node *node){
	int i = 0, j=0;
	char *str;
	Link *link;
	verbose(2, "Number of nodes in Graph: %d\n", graphLength(node));
	verbose(2, "------------------------------\n");

	while(node != NULL){
		verbose(2, "| %d |  ip: %s\tsequence #: %d\n", i, getucharstr(node->ip), node->seq_Numb);

		link = node->list;
		j=0;
		verbose(2, "|\tNumber of neighbours of node %d: %d\n", i, neighbourListLength(node));
		verbose(2, "|\t--------------------------------\n");
		while(link!=NULL){
			verbose(2, "|\t| %d :\tLink ID: %s\n", j, getucharstr(link->linkId));
			verbose(2, "|\t|\tLink Data: %s\n", getucharstr(link->linkData));
			verbose(2, "|\t|\tLink Type: %d\n", link->linkType);
			link = link->next;
			j++;
		}
		verbose(2, "|\n");
		node = node->next;
		i++; 	
	}

	verbose(2, "\n");
}

//Obtained the lowest distance in the distance array (for dijkstra algorithm)
int getNodeWithLowestDistance(int distance[], int observed[], int numbNodes){
	int result = -1, smallestValue = INF, i;

	for(i=0;i<numbNodes;i++){
		if(observed[i]==1)
			continue;
		if(distance[i] < smallestValue){
			smallestValue=distance[i];
			result = i;
		}
	}

	return result;
}

//Find the node ID in the node list
int getNodeId(Link *neighbour, Node nodes[], int numbNodes){
	int i;
	//verbose(2, "Looking for node with ip: %s\n", getucharstr(neighbour->linkData));
	for(i = 0; i<numbNodes; i++){
		//verbose(2, "\t Node Ip: %s\n", getucharstr(nodes[i].ip));
		if((COMPARE_IP(neighbour->linkData, nodes[i].ip)==0))
			return i;
	}

	//Problem, cannot find router in list
	verbose(2, "Problem, cannot find router %s in list.\n", getucharstr(neighbour->linkData));
	return -1;
}

//Recursive function that finds the next router in the router list --> returns router list ID
int calcNextHop(int index, int distance[], int previousNode[]){
	if(distance[previousNode[index]] == INF) return -1;

	if(distance[previousNode[index]] == 0) return index;

	return calcNextHop(previousNode[index], distance, previousNode);
}


//Function that checks if the IP of the node corresponds to one IP of an interface
//Returns 1 if true (if IP address corresponds to one of the interface of the router), 0 if false
int findCorrectInterface(uchar *ip, uchar interfaces[][4], int numbInterface){
	int i = 0;

	for(i=0;i<numbInterface;i++){
		if((ip[1]==interfaces[i][1])&&(ip[2]==interfaces[i][2])&&(ip[3]==interfaces[i][3]))
			return i;	
	}

	return -1;
}

//Function that checks if the IP of the node corresponds to one IP of an interface
//Returns 1 if true (if IP address corresponds to one of the interface of the router), 0 if false
int isNodeAnInterfaceOfRouter(char *ip, uchar interfaces[][4], int numbInterface){
	int i = 0;

	for(i=0;i<numbInterface;i++){
		if(COMPARE_IP(ip,interfaces[i]) == 0)
			return 1;	
	}

	return 0;
}

int nextHopListLength(NextHop *list){
	int i = 0;

	while(list!=NULL){
		list = list->next;
		i++;
	}

	return i;
}

//Take a list of nexthop and remove redundancy by keeping best solutions
NextHop* removeRedundancy(NextHop *list){
	int listLength = nextHopListLength(list), i, j;

	if(listLength == 0){
		verbose(2, "Returning a null List.\n");
		return NULL;
	}else if(listLength == 1){
		verbose(2, "Returning original list.\n");
		return list;
	}

	NextHop nextHops[listLength];
	int include[listLength];

	NextHop *tempList = list;

	for(i = 0; i < listLength ; i++){
		nextHops[i] = *tempList;
		include[i] = 1;
		tempList = tempList->next;
	}

	for(i = 0; i < listLength ; i++){
		//verbose(2, "Now checking for node: %s/(%s)\n", );
		if(i!=(listLength-1))
		{
			for(j = i+1; j<listLength; j++){
				if(include[j] == 0){
					verbose(2, "Skip this one as found a better one.");
					continue;
				}
				if(COMPARE_IP(nextHops[i].rnetwork, nextHops[j].rnetwork) == 0 && COMPARE_IP(nextHops[i].rsubmask, nextHops[j].rsubmask)==0){
					verbose(2, "We have a fucking match now remove on for fucks sake.\n");
					verbose(2, "Comparing %s/(%s): %d,  with %s/(%s): %d.\n", getucharstr(nextHops[i].rnetwork), getucharstr(nextHops[i].rsubmask),
						nextHops[i].distance, getucharstr(nextHops[j].rnetwork), getucharstr(nextHops[j].rsubmask), nextHops[j].distance);
					if(nextHops[i].distance >= nextHops[j].distance){
						verbose(2, "Removing one node: %s/(%s)\n", getucharstr(nextHops[i].rnetwork), getucharstr(nextHops[i].rsubmask));
						include[i] = 0;
						break;
					}else{
						verbose(2, "Removing one node: %s/(%s)\n", getucharstr(nextHops[j].rnetwork), getucharstr(nextHops[j].rsubmask));
						include[j]=0;
					}
				}
			}
		}
	}

	NextHop *newList = NULL;

	for(i=0;i<listLength;i++){
		if(include[i] == 1){
			NextHop *newNode = (NextHop *)malloc(sizeof(NextHop));
			COPY_IP(newNode->rnetwork, nextHops[i].rnetwork);
			COPY_IP(newNode->rsubmask, nextHops[i].rsubmask);
			COPY_IP(newNode->nh_ip, nextHops[i].nh_ip);
			COPY_IP(newNode->interfaceIp, nextHops[i].interfaceIp);
			newNode->distance = nextHops[i].distance;

			newNode->next = newList;
			newList = newNode;
		}
	}

	return newList;
}


//Well, given a list of nodes and a source node, calculates the dijkstra algorithm
NextHop* calculateDijkstra(Node *head, uchar interfaces[][4], int numbInterface){
	//First set all necessary variables for this dijkstra calculation
	Node *nodes = head;
	int numbNodes = graphLength(nodes), i, j, currentNodeId = 0, id;
	Node node[numbNodes];
	int distance[numbNodes], seen[numbNodes], numbSeen = 0;
	int previousNode[numbNodes];

	//Create NextHopList to add stub network when finding them while computing dijkstra algorithm
	NextHop *nextHopList = NULL;
	StubHop *stubHops = NULL;

	printGraphJeremie(head);

	//Set all variables inside all arrays
	for(i=0;i<numbNodes;i++){
		node[i] = *nodes;
		distance[i] = INF;
		//if(strcmp(nodes->router->ip, sourceRouter->ip) == 0)
		if((isNodeAnInterfaceOfRouter(nodes->ip, interfaces, numbInterface) == 1))
		{
			verbose(2, "Set Distance of node %d: %s to 0.\n", i, getucharstr(nodes->ip));
			distance[i] = 0;
			//currentNodeId = i;
		}

		seen[i] = 0;
		previousNode[i] = -1;

		nodes = nodes->next;
	}
	//verbose(2, "i = %d, distance of source \"%s\" is of %d\n",currentNodeId, getucharstr(sourceNode->ip), distance[currentNodeId]);

	do{
		verbose(2, "\n");
		currentNodeId = getNodeWithLowestDistance(distance, seen, numbNodes);

		//This implies that there was no path found to get to the destination node
		if(currentNodeId == -1) {
			verbose(2, "There are no more possible connections between routers. Ending algorithm\n");
			numbSeen = numbNodes;
			continue;
		}

		verbose(2, "Smallest node is now %d with # hops distance == %d from source.\n",currentNodeId, distance[currentNodeId]);
		
		numbSeen++;
		seen[currentNodeId] = 1;//turn on flag to tell algorithm that we have already gone through this node

		i = neighbourListLength(&(node[currentNodeId]));
		verbose(2, "Node %s has %d neighbours.\n", getucharstr(node[currentNodeId].ip), i);

		Link *neighbour = node[currentNodeId].list;

		while(neighbour != NULL){
			//First check if neighbour represent and network stub
			if(neighbour->linkType == 3){
				//Add network stub to NextHop list
				StubHop *hop = (StubHop *)malloc(sizeof(StubHop));
				
				COPY_IP(hop->rnetwork, neighbour->linkId);
				Dot2IP("255.255.255.0", hop->rsubmask);
				hop->assNode = currentNodeId;//reference node that will have the same nextHop

				hop->next = stubHops;
				stubHops = hop;
			}else{//Has type 2

				id = getNodeId(neighbour, node, numbNodes);
				if(id == -1){
					verbose(2, "There is no recorded router for neighbour %s. Path ends here.", getucharstr(neighbour->linkData));

					StubHop *hop = (StubHop *)malloc(sizeof(StubHop));
				
					COPY_IP(hop->rnetwork, neighbour->linkData);
					Dot2IP("255.255.255.0", hop->rsubmask);
					hop->assNode = currentNodeId;//reference node that will have the same nextHop

					hop->next = stubHops;
					stubHops = hop;
				}else{

					verbose(2, "Id of node %s is : %d\n", getucharstr(neighbour->linkData), id);
					verbose(2, "Distance of node %d is: %d, whereas distance of node %d +1 is: %d\n", id, distance[id], currentNodeId, distance[currentNodeId]+1);
					if(distance[id] > (distance[currentNodeId] + 1)){//If we find a shorter path to this node
						distance[id] = distance[currentNodeId] + 1;
						previousNode[id] = currentNodeId;
						verbose(2, "Changing distance of node %s\n", getucharstr(neighbour->linkData));
					}
				}
			}

			neighbour = neighbour->next;
		}
	}while(numbSeen<numbNodes);

	//Dijkstra stopped running
	verbose(2, "\n\nAlgorithm has stopped running. Calculating paths for each nodes.\n\n");

	//verbose(2, "Source Node: %s\n", getucharstr(sourceNode->ip));

	for(i=0;i<numbNodes;i++){
		//We don't calculate nextHop for our router
		if(distance[i] == 0)
			continue;
		//If we have infinity still left in a node, then this node is unreachable
		if(distance[i] == INF){
			verbose(2, "\t|Distance of node %s is INFINITY. This node is unreachable from source.", getucharstr(node[i].ip));
			continue;
		}


		id = calcNextHop(i, distance, previousNode);

		verbose(2, "\t|To get to%s it takes %d hops. \n\t|\tNext hop to reach %s is: %s\n",
			getucharstr(node[i].ip), distance[i], getucharstr(node[i].ip),getucharstr(node[id].ip));



		NextHop *next = (NextHop *)malloc(sizeof(NextHop));

		next->rnetwork[0] = 0;
		next->rnetwork[1] = node[i].ip[1];
		next->rnetwork[2] = node[i].ip[2];
		next->rnetwork[3] = node[i].ip[3];

		Dot2IP("255.255.255.0", next->rsubmask);

		if(distance[i] == 1){
			//We are right next to the network
			Dot2IP("0.0.0.0", next->nh_ip);
			COPY_IP(next->neighbourIpHack, node[id].ip) ;
			COPY_IP(next->rsubmask, node[id].ip);
		}else{
			COPY_IP(next->nh_ip, node[id].ip);
			/*j = findCorrectInterface(node[id].ip, interfaces, numbInterface);
			if(j == -1){
				verbose(2, "ERROR: INTERFACE # == -1 skipping this element");
				continue;
			}
			COPY_IP(next->interfaceIp, interfaces[j]);*/
		}

		j = findCorrectInterface(node[id].ip, interfaces, numbInterface);
		if(j == -1){
			verbose(2, "ERROR: INTERFACE # == -1 skipping this element");
			continue;
		}
		COPY_IP(next->interfaceIp, interfaces[j]);


		next->distance = distance[i];

		next->next = nextHopList;
		nextHopList = next;



		verbose(2, "\n");
	}

	while(stubHops!=NULL){

		if(previousNode[stubHops->assNode] == -1)
		{
			NextHop *next = (NextHop *)malloc(sizeof(NextHop));

			next->rnetwork[0] = 0;
			next->rnetwork[1] = stubHops->rnetwork[1];
			next->rnetwork[2] = stubHops->rnetwork[2];
			next->rnetwork[3] = stubHops->rnetwork[3];

			COPY_IP(next->rsubmask, stubHops->rsubmask);
			
			Dot2IP("0.0.0.0", next->nh_ip);
			//COPY_IP(next->nh_ip, stubHops->rnetwork);

			j = findCorrectInterface(stubHops->rnetwork, interfaces, numbInterface);
			if(j == -1){
				verbose(2, "ERROR: INTERFACE # == -1 skipping this element");
				continue;
			}
			COPY_IP(next->interfaceIp, interfaces[j]);

			COPY_IP(next->rsubmask, stubHops->rnetwork) ;

			//COPY_IP(next->interfaceIp, node[stubHops->assNode].ip);

			next->next = nextHopList;
			nextHopList = next;

			next->distance = distance[stubHops->assNode] + 1;

			stubHops = stubHops->next;
			continue;
		}

		//Create new NextHop
		NextHop *next = (NextHop *)malloc(sizeof(NextHop));

		next->rnetwork[0] = 0;
		next->rnetwork[1] = stubHops->rnetwork[1];
		next->rnetwork[2] = stubHops->rnetwork[2];
		next->rnetwork[3] = stubHops->rnetwork[3];

		COPY_IP(next->rsubmask, stubHops->rsubmask);

		id = calcNextHop(stubHops->assNode, distance, previousNode);

		COPY_IP(next->nh_ip, node[id].ip);

		j = findCorrectInterface(node[id].ip, interfaces, numbInterface);
		if(j == -1){
			verbose(2, "ERROR: INTERFACE # == -1 skipping this element");
			continue;
		}
		COPY_IP(next->interfaceIp, interfaces[j]);
		//COPY_IP(next->interfaceIp, node[previousNode[id]].ip);

		next->distance = distance[stubHops->assNode] + 1;

		stubHops = stubHops->next;

		next->next = nextHopList;
		nextHopList = next;
	}

	verbose(2, "\n\n");

	verbose(2, "We have a total of %d nexthops.\n",nextHopListLength(nextHopList));

	//TODO: Check for redundancy:
	nextHopList = removeRedundancy(nextHopList);

	verbose(2, "After algorithm, we have a total of %d nexthops.\n",nextHopListLength(nextHopList));
	//
	//

	//printNextHopList(nextHopList);

	return nextHopList;
}

void printNextHopList(NextHop *list){
	while(list!=NULL){
		verbose(2, "Network %s with submask %s has next hop going to %s with distance of %d\n", 
			getucharstr(list->rnetwork), getucharstr(list->rsubmask), getucharstr(list->nh_ip), list->distance);
		verbose(2, "\t| Interface ip to send through: %s\n\n", getucharstr(list->interfaceIp));
		list = list->next;
	}
}

void printGraph(Node *graph) {
	uchar tmpbuf[MAX_TMPBUF_LEN];
	Node *curNode = graph;
	printf( "------------------------------------------------------------------\n");
	printf( " O S P F :  T O P O L O G Y   G R A P H  \n");
	printf( "------------------------------------------------------------------\n");
	while (curNode != NULL) {
		printf( "Node IP: %s\n", IP2Dot(tmpbuf, curNode->ip));
		printf( "Sequence Number: %d\n", curNode->seq_Numb);
		printf( "Links:\n");
		Link *curLink = curNode->list;
		while(curLink != NULL) {
			printf( "\tLink ID: %s\n", IP2Dot(tmpbuf, curLink->linkId));
			printf( "\tLink Data: %s\n", IP2Dot(tmpbuf, curLink->linkData));
			printf( "\tType: %s\n\n", (curLink->linkType == TYPE_ANY_TO_ANY) ? "Any-to-any" : "Stub");
			curLink = curLink->next;
		}
		curNode = curNode->next;
	}
}

void printNextHops(NextHop *head) {
	verbose(2, "------------------------------------------------------------------\n");
	verbose(2, " O S P F :  N E X T   H O P S  \n");
	verbose(2, "------------------------------------------------------------------\n");
	verbose(2, "Network Address\tSubnet Mask\tNext Hop\n");

	uchar tmpbuf[MAX_TMPBUF_LEN];
	NextHop *cur = head;
	while (cur != NULL) {
		verbose(2, "%s\t", IP2Dot(tmpbuf, cur->rnetwork));
		verbose(2, "%s\t", IP2Dot(tmpbuf, cur->rsubmask));
		verbose(2, "%s\t\n", IP2Dot(tmpbuf, cur->nh_ip));
		cur = cur->next;
	}
}

/* Finds a node in the graph with the given IP. Returns NULL if no match. */
Node *getNodeByIP(Node *graph, uchar ip[]) {
	uchar tmpbuf[MAX_TMPBUF_LEN];
	Node *cur = graph;
	while(cur != NULL) {
		if (!COMPARE_IP(cur->ip, ip)) {
			break;
		}
		cur = cur->next;
	}
	return cur;
}

/* Adds a node and returns the head of the list. */
Node *addNode(Node *graph, Node *newNode) {
	if (graph != NULL) { newNode->next = graph; }
	else { newNode->next = NULL; }
	return newNode;
}

/* Adds a link and returns the head of the list. */
Link *addLink(Link *list, Link *newLink) {
	if (list != NULL) { newLink->next = list; }
	else { newLink->next = NULL; }
	return newLink;
}

/* Removes the first link in the list of links with the specified link data and returns the head of the list */
Link *removeLinkByLinkData(Link *list, uchar linkData[]) {
	Link *cur = list;

	if (list == NULL) return list;

	if (!COMPARE_IP(cur->linkData, linkData)) {
		// Remove the head of the list
		list = list->next;
		free(cur);
		return list;
	}

	while (cur->next != NULL) {
		Link *prev = cur;
		cur = cur->next;
		if (!COMPARE_IP(cur->linkData, linkData)) {
			prev->next = cur->next;
			free(cur);
			break;
		}
	}

	return list;
}

/* Removes the first node in the graph with the specified ip and returns the head of the graph */
Node *removeNodeByIP(Node *graph, uchar ip[]) {
	Node *cur = graph;

	if (graph == NULL) return graph;

	if (!COMPARE_IP(cur->ip, ip)) {
		// Remove the head of the list
		graph = graph->next;
		free(cur);
		return graph;
	}

	while (cur->next != NULL) {
		Node *prev = cur;
		cur = cur->next;
		if (!COMPARE_IP(cur->ip, ip)) {
			prev->next = cur->next;
			free(cur);
			break;
		}
	}

	return graph;
}
