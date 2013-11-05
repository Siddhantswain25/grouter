/*
 * Operations on the topology graph
 */
#include "graph.h"

void printuchar(uchar ip[]){
	printf("%d.%d.%d.%d", ip[3], ip[2], ip[1],ip[0]);
}


char* getucharstr(uchar ip[]){
	char* str = (char*)malloc(sizeof(char)*16);
	sprintf(str, "%d.%d.%d.%d", ip[3], ip[2], ip[1], ip[0]);

	///printf("%s", str);

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
		//printf("Element found: %s\n",list->router->ip);
		node = node->next;
		length++;
	}
	//printf("Length of list is of : %d\n\n", length);
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
	printf("Number of nodes in Graph: %d\n", graphLength(node));
	printf("------------------------------\n");

	while(node != NULL){
		printf("| %d |  ip: %s\tsequence #: %d\n", i, getucharstr(node->ip), node->seq_Numb);

		link = node->list;
		j=0;
		printf("|\tNumber of neighbours of node %d: %d\n", i, neighbourListLength(node));
		printf("|\t--------------------------------\n");
		while(link!=NULL){
			printf("|\t| %d :\tLink ID: %s\n", j, getucharstr(link->linkId));
			printf("|\t|\tLink Data: %s\n", getucharstr(link->linkData));
			printf("|\t|\tLink Type: %d\n", link->linkType);
			link = link->next;
			j++;
		}
		printf("|\n");
		node = node->next;
		i++; 	
	}

	printf("\n");
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
	//printf("Looking for node with ip: %s\n", getucharstr(neighbour->linkData));
	for(i = 0; i<numbNodes; i++){
		//printf("\t Node Ip: %s\n", getucharstr(nodes[i].ip));
		if((COMPARE_IP(neighbour->linkData, nodes[i].ip)==0))
			return i;
	}

	//Problem, cannot find router in list
	printf("Problem, cannot find router %s in list.\n", getucharstr(neighbour->linkData));
	return -1;
}

//Recursive function that finds the next router in the router list --> returns router list ID
int calcNextHop(int index, int distance[], int previousNode[]){
	if(distance[previousNode[index]] == INF) return -1;

	if(distance[previousNode[index]] == 0) return index;

	return calcNextHop(previousNode[index], distance, previousNode);
}


//Well, given a list of nodes and a source node, calculates the dijkstra algorithm
NextHop* calculateDijkstra(Node *head, Node *sourceNode){
	//First set all necessary variables for this dijkstra calculation
	Node *nodes = head;
	int numbNodes = graphLength(nodes), i, currentNodeId = 0, id;
	Node node[numbNodes];
	int distance[numbNodes], seen[numbNodes], numbSeen = 0;
	int previousNode[numbNodes];

	//Create NextHopList to add stub network when finding them while computing dijkstra algorithm
	NextHop *nextHopList = NULL;
	StubHop *stubHops = NULL;

	//Set all variables inside all arrays
	for(i=0;i<numbNodes;i++){
		node[i] = *nodes;
		distance[i] = INF;
		//if(strcmp(nodes->router->ip, sourceRouter->ip) == 0)
		if((COMPARE_IP(nodes->ip, sourceNode->ip) == 0))
		{
			printf("Set Distance of node %d: %s to 0.\n", i, getucharstr(nodes->ip));
			distance[i] = 0;
			currentNodeId = i;
		}

		seen[i] = 0;
		previousNode[i] = -1;

		nodes = nodes->next;
	}
	//printf("i = %d, distance of source \"%s\" is of %d\n",currentNodeId, getucharstr(sourceNode->ip), distance[currentNodeId]);

	do{
		printf("\n");
		currentNodeId = getNodeWithLowestDistance(distance, seen, numbNodes);

		//This implies that there was no path found to get to the destination node
		if(currentNodeId == -1) {
			printf("ERROR: Impossible to get to node %s, ending algorithm.", getucharstr(node[currentNodeId].ip));
			return NULL;
		}

		printf("Smallest node is now %d with # hops distance == %d from source.\n",currentNodeId, distance[currentNodeId]);
		
		numbSeen++;
		seen[currentNodeId] = 1;//turn on flag to tell algorithm that we have already gone through this node

		i = neighbourListLength(&(node[currentNodeId]));
		printf("Node %s has %d neighbours.\n", getucharstr(node[currentNodeId].ip), i);

		Link *neighbour = node[currentNodeId].list;

		while(neighbour != NULL){
			//First check if neighbour represent and network stub
			if(neighbour->linkType == 3){
				//Add network stub to NextHop list
				StubHop *hop = (StubHop *)malloc(sizeof(StubHop));
				//hop->rnetwork = *(neighbour->linkId);
				//char *tmp;
				//Dot2IP(IP2Dot(tmp, neighbour->linkId), hop->rnetwork);
				hop->rnetwork[0] = neighbour->linkId[0]; 
				hop->rnetwork[1] = neighbour->linkId[1]; 
				hop->rnetwork[2] = neighbour->linkId[2]; 
				hop->rnetwork[3] = neighbour->linkId[3]; 

				//hop->rsubmask = neighbour->linkData;
				//Dot2IP(IP2Dot(tmp, neighbour->linkData), hop->rsubmask);
				hop->rsubmask[0] = neighbour->linkData[0]; 
				hop->rsubmask[1] = neighbour->linkData[1]; 
				hop->rsubmask[2] = neighbour->linkData[2]; 
				hop->rsubmask[3] = neighbour->linkData[3];

				hop->assNode = currentNodeId;//reference node that will have the same nextHop

				hop->next = stubHops;
				stubHops = hop;
			}else{//Has type 2

				id = getNodeId(neighbour, node, numbNodes);
				printf("Id of node %s is : %d\n", getucharstr(neighbour->linkData), id);
				printf("Distance of node %d is: %d, whereas distance of node %d +1 is: %d\n", id, distance[id], currentNodeId, distance[currentNodeId]+1);
				if(distance[id] > (distance[currentNodeId] + 1)){//If we find a shorter path to this node
					distance[id] = distance[currentNodeId] + 1;
					previousNode[id] = currentNodeId;
					printf("Changing distance of node %s\n", getucharstr(neighbour->linkData));
				}
			}

			neighbour = neighbour->next;
		}
	}while(numbSeen<numbNodes);

	//Dijkstra stopped running
	printf("\n\nAlgorithm has stopped running. Calculating paths for each nodes.\n\n");

	printf("Source Node: %s\n", getucharstr(sourceNode->ip));

	for(i=0;i<numbNodes;i++){
		//We don't calculate nextHop for our router
		if(distance[i] == 0)
			continue;

		//If we have infinity still left in a node, then this node is unreachable
		if(distance[i] == INF)
			printf("\t|Distance of node %s is INFINITY. This node is unreachable from source.", getucharstr(node[i].ip));


		id = calcNextHop(i, distance, previousNode);

		printf("\t|%s to %s takes %d hops. \n\t|\tNext hop to reach %s is: %s\n",
		 getucharstr(sourceNode->ip), getucharstr(node[i].ip), distance[i], getucharstr(node[i].ip),getucharstr(node[id].ip));



		NextHop *next = (NextHop *)malloc(sizeof(NextHop));

		//Dot2IP(tmp, next->rnetwork);
		next->rnetwork[0] = 0;
		next->rnetwork[1] = node[i].ip[1];
		next->rnetwork[2] = node[i].ip[2];
		next->rnetwork[3] = node[i].ip[3];

		Dot2IP("255.255.255.0", next->rsubmask);

		//Dot2IP(IP2Dot(tmp, node[id].ip), next->nh_ip);
		next->nh_ip[0] = node[id].ip[0];
		next->nh_ip[1] = node[id].ip[1];
		next->nh_ip[2] = node[id].ip[2];
		next->nh_ip[3] = node[id].ip[3];

		next->next = nextHopList;
		nextHopList = next;



		printf("\n");
	}

	while(stubHops!=NULL){

		if(previousNode[stubHops->assNode] == -1)
		{
			stubHops = stubHops->next;
			continue;
		}

		NextHop *next = (NextHop *)malloc(sizeof(NextHop));
		next->rnetwork[0] = 0;
		next->rnetwork[1] = stubHops->rnetwork[1];
		next->rnetwork[2] = stubHops->rnetwork[2];
		next->rnetwork[3] = stubHops->rnetwork[3];

		next->rsubmask[0] = stubHops->rsubmask[0];
		next->rsubmask[1] = stubHops->rsubmask[1];
		next->rsubmask[2] = stubHops->rsubmask[2];
		next->rsubmask[3] = stubHops->rsubmask[3];

		//printf("Id stored inside stubHops: %d\n", stubHops->assNode);


		id = calcNextHop(stubHops->assNode, distance, previousNode);

		next->nh_ip[0] = node[id].ip[0];
		next->nh_ip[1] = node[id].ip[1];
		next->nh_ip[2] = node[id].ip[2];
		next->nh_ip[3] = node[id].ip[3];

		stubHops = stubHops->next;

		next->next = nextHopList;
		nextHopList = next;
	}

	printf("\n\n");

	return nextHopList;
}

void printNextHopList(NextHop *list){
	while(list!=NULL){
		printf("Network %s with submask %s has next hop going to %s\n\n", getucharstr(list->rnetwork), getucharstr(list->rsubmask), getucharstr(list->nh_ip));
		list = list->next;
	}
}

void printGraph(Node *graph) {
	uchar tmpbuf[MAX_TMPBUF_LEN];
	Node *curNode = graph;
	printf("------------------------------------------------------------------\n");
	printf(" O S P F :  T O P O L O G Y   G R A P H  \n");
	printf("------------------------------------------------------------------\n");
	while (curNode != NULL) {
		printf("Node IP: %s\n", IP2Dot(tmpbuf, curNode->ip));
		printf("Sequence Number: %d\n", curNode->seq_Numb);
		printf("Links:\n");
		Link *curLink = curNode->list;
		while(curLink != NULL) {
			printf("\tLink ID: %s\n", IP2Dot(tmpbuf, curLink->linkId));
			printf("\tLink Data: %s\n", IP2Dot(tmpbuf, curLink->linkData));
			printf("\tType: %s\n\n", (curLink->linkType == TYPE_ANY_TO_ANY) ? "Any-to-any" : "Stub");
			curLink = curLink->next;
		}
		curNode = curNode->next;
	}
}

void printNextHops(NextHop *head) {
	printf("------------------------------------------------------------------\n");
	printf(" O S P F :  N E X T   H O P S  \n");
	printf("------------------------------------------------------------------\n");
	printf("Network Address\tSubnet Mask\tNext Hop\n");

	uchar tmpbuf[MAX_TMPBUF_LEN];
	NextHop *cur = head;
	while (cur != NULL) {
		printf("%s\t", IP2Dot(tmpbuf, cur->rnetwork));
		printf("%s\t", IP2Dot(tmpbuf, cur->rsubmask));
		printf("%s\t\n", IP2Dot(tmpbuf, cur->nh_ip));
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
