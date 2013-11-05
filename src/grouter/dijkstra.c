#include "dijkstra.h"

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
void printGraph(Node *node){
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
int getLowestDistance(int distance[], int observed[], int numbNodes){
	int result = -1, smallestValue = INFINITY, i;

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

//Recursive function that finds the next router in the router list --> returns router list ID
int calcNextHop(int index, int distance[], int previousNode[]){
	if(distance[previousNode[index]] == INFINITY) return -1;

	if(distance[previousNode[index]] == 0) return index;

	return calcNextHop(previousNode[index], distance, previousNode);
}


//Constant declaration
/*#define INFINITY	99999

typedef unsigned char uchar;

//Predeclared Structs
typedef struct _Router Router;
typedef struct _Node Node;

//Predeclared Functions
void printList(Node *list);

//------STRUCTS--------
typedef struct _Node{
	Router *router;
	Node *next;
}Node;

typedef struct _Router{
	uchar submask[4];
	uchar network[4];
	Node *list;
}Router;*/

/*
void printuchar(uchar ip[]){
	printf("%d.%d.%d.%d", ip[0], ip[1], ip[2],ip[3]);
}



char* getucharstr(uchar ip[]){
	char* str = (char*)malloc(sizeof(char)*16);
	sprintf(str, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

	///printf("%s", str);

	return str;	
}

//-------FUNCTIONS------------
//Returns the length of a given list
int length(Node *givenList){
	int length = 0;
	Node *list;
	list = givenList;
	while(list != NULL){
		//printf("Element found: %s\n",list->router->ip);
		list = list->next;
		length++;
	}
	//printf("Length of list is of : %d\n\n", length);
	return length;
}

//Add router rToAdd to router addToThisR's list
void add(Router *rToAdd, Router *addToThisR){
	Node *newList = (Node *)malloc(sizeof(Node));
	Node *next;
	next = addToThisR->list;
	newList->next = next;
	newList->router = rToAdd;

	length(newList);

	//Ensures list at given router is now proper good brov --> yeah chavvv way blad
	addToThisR->list = newList;

	printf("Length of list of router net:%s/(%s) is now: %d\n", getucharstr(addToThisR->network),getucharstr(addToThisR->submask), length(newList));
	//Print list for debugging purposes
	printList(newList);
}

//Given a list, prints all elements contained in list
void printList(Node *list){
	int i = 1;
	printf("List elements: \n");

	while(list != NULL){
		printf("\t|  %d  -  %s\t%s\n", i, getucharstr(list->router->network), getucharstr(list->router->submask));
		list = list->next;
		i++;
	}

	printf("\n");
}

//Obtained the lowest distance in the distance array (for dijkstra algorithm)
int getLowestDistance(int distance[], int observed[], int numbNodes){
	int result = -1, smallestValue = INFINITY, i;

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

//Find the router ID in the routers list
int getNodeId(Router *router, Router routers[], int numbNodes){
	int i;
	for(i = 0; i<numbNodes; i++){
		if((COMPARE_IP(router->network, routers[i].network)==0) && (COMPARE_IP(router->submask, routers[i].submask)==0))
			return i;
	}

	//Problem, cannot find router in list
	printf("Problem, cannot find router %s/(%s) in list.\n", getucharstr(router->network), getucharstr(router->submask));
	return -1;
}

//Recursive function that finds the next router in the router list --> returns router list ID
int calcNextHop(int index, int distance[], int previousNode[]){
	if(distance[previousNode[index]] == INFINITY) return -1;

	if(distance[previousNode[index]] == 0) return index;

	return calcNextHop(previousNode[index], distance, previousNode);
}

//Well, given a list of nodes and a source node, calculates the dijkstra algorithm
NextHop* calculateDijkstra(Router *root, Router *sourceRouter){
	//First set all necessary variables for this dijkstra calculation
	Node *nodes = root->list;
	int numbNodes = length(nodes), i, currentNode = 0, id;
	Router router[numbNodes];
	int distance[numbNodes], seen[numbNodes], numbSeen = 0;
	int previousNode[numbNodes];

	for(i=0;i<numbNodes;i++){
		router[i] = *(nodes->router);
		distance[i] = INFINITY;
		//if(strcmp(nodes->router->ip, sourceRouter->ip) == 0)
		if((COMPARE_IP(nodes->router->network, sourceRouter->network) == 0) && (COMPARE_IP(nodes->router->submask, sourceRouter->submask) == 0))
		{
			distance[i] = 0;
			currentNode = i;
		}

		seen[i] = 0;
		previousNode[i] = -1;

		nodes = nodes->next;
	}
	printf("i = %d, distance of source \"%s/(%s)\" is of %d\n",i, getucharstr(sourceRouter->network), getucharstr(sourceRouter->submask), distance[currentNode]);

	do{
		currentNode = getLowestDistance(distance, seen, numbNodes);
		if(currentNode == -1) break;//This implies that there was no path found to get to the destination node
		printf("Smallest node is now %d with distance of %d\n",currentNode, distance[currentNode]);
		numbSeen++;
		seen[currentNode] = 1;


		i = length(router[currentNode].list);
		printf("Number of neighbours of node .%s/(%s) : %d\n", getucharstr(router[currentNode].network), getucharstr(router[currentNode].submask), i);

		Node *temp = router[currentNode].list;

		while(temp!=NULL){
			id = getNodeId(temp->router, router, numbNodes);
			printf("Id of node %s/(%s) is : %d\n", getucharstr(temp->router->network), getucharstr(temp->router->submask), id);
			printf("Distance of node %d is: %d, whereas distance of node %d +1 is: %d\n", id, distance[id], currentNode, distance[currentNode]+1);
			if(distance[id] > (distance[currentNode] + 1)){//If we find a shorter path to this node
				distance[id] = distance[currentNode] + 1;
				previousNode[id] = currentNode;
				printf("Changing distance of node %s/(%s)\n", getucharstr(temp->router->network), getucharstr(temp->router->submask));
			}

			temp = temp->next;
		}
	}while(numbSeen<numbNodes);

	//Dijkstra stopped running
	printf("\n\nAlgorithm has stopped running. Calculating paths for each nodes.\n\n");

	printf("Source Node: %s/(%s)\n", getucharstr(sourceRouter->network), getucharstr(sourceRouter->submask));

	NextHop *nextHopList = NULL;

	for(i=0;i<numbNodes;i++){
		if(distance[i] == 0)
			continue;
		if(distance[i] == INFINITY)
			printf("\t|Distance of node %s/(%s) is INFINITY. This node is unreachable from source.", getucharstr(router[i].network), getucharstr(router[i].submask));

		id = calcNextHop(i, distance, previousNode);

		NextHop *next = (NextHop *)malloc(sizeof(NextHop));

		next->rnetwork[0] = router[i].network[0];
		next->rnetwork[1] = router[i].network[1];
		next->rnetwork[2] = router[i].network[2];
		next->rnetwork[3] = router[i].network[3];

		next->rsubmask[0] = router[i].submask[0];
		next->rsubmask[1] = router[i].submask[1];
		next->rsubmask[2] = router[i].submask[2];
		next->rsubmask[3] = router[i].submask[3];

		next->nh_ip[0] = router[id].network[0];
		next->nh_ip[1] = router[id].network[1];
		next->nh_ip[2] = router[id].network[2];
		next->nh_ip[3] = router[id].network[3];

		next->next = nextHopList;

		nextHopList = next;


		/*printf("\t|%s/(%s) to %s/(%s) takes %d hops. \n\t\tNext hop to reach %s/(%s) is: %s\n",
		 getucharstr(sourceRouter->network), getucharstr(sourceRouter->submask), 
		 getucharstr(router[i].network), getucharstr(router[i].submask), distance[i], 
		 getucharstr(router[i].network), getucharstr(router[i].submask), 
		 getucharstr(router[id].network), getucharstr(router[id].submask));
	}
/*
	printf("\n\n");
	//printf("TODO: Update Forwarding Table");

	return nextHopList;
}

int test(){	

	//First Setup
	/*

	Router *root = (Router *)malloc(sizeof(Router));

	//Populate Nodes for Dijkstra test
	Router *a = (Router *)malloc(sizeof(Router));
	Router *b = (Router *)malloc(sizeof(Router));
	Router *c = (Router *)malloc(sizeof(Router));
	Router *d = (Router *)malloc(sizeof(Router));
	Router *e = (Router *)malloc(sizeof(Router));
	Router *f = (Router *)malloc(sizeof(Router));
	Router *g = (Router *)malloc(sizeof(Router));
	Router *h = (Router *)malloc(sizeof(Router));
	Router *i = (Router *)malloc(sizeof(Router));
	Router *j = (Router *)malloc(sizeof(Router));
	Router *k = (Router *)malloc(sizeof(Router));
	Router *l = (Router *)malloc(sizeof(Router));

	a->ip = "a";
	b->ip = "b";
	c->ip = "c";
	d->ip = "d";
	e->ip = "e";
	f->ip = "f";
	g->ip = "g";
	h->ip = "h";
	i->ip = "i";
	j->ip = "j";
	k->ip = "k";
	l->ip = "l";

	add(a,root);
	add(b,root);
	add(c,root);
	add(d,root);
	add(e,root);
	add(f,root);
	add(g,root);
	add(h,root);
	add(i,root);
	add(j,root);
	add(k,root);
	add(l,root);

	//Adding neighbour nodes to a
	add(b,a);
	add(c,a);

	//Adding neightbour nodes to b
	add(a,b);
	add(c,b);
	add(d,b);
	add(g,b);

	//Adding neightbour nodes to c
	add(a,c);
	add(b,c);
	add(d,c);

	//Adding neightbour nodes to d
	add(b,d);
	add(c,d);
	add(i,d);
	add(e,d);

	//Adding neightbour nodes to e
	add(d,e);
	add(f,e);

	//Adding neightbour nodes to f
	add(e,f);
	add(g,f);
	add(h,f);

	//Adding neightbour nodes to g
	add(f,g);
	add(b,g);

	//Adding neightbour nodes to h
	add(f,h);
	add(k,h);

	//Adding neightbour nodes to i
	add(d,i);
	add(j,i);
	add(l,i);
	add(k,i);

	//Adding neightbour nodes to j
	add(i,j);

	//Adding neightbour nodes to k
	add(i,k);
	add(l,k);
	add(h,k);

	//Adding neightbour nodes to l
	add(k,l);
	add(i,l);


	calculateDijkstra(root, a);


	uchar test[4];
	test[0] = 192;
	test[1] = 168;
	test[2] = 1;
	test[3] = 1;

	printf("\n\nPrinting test uchar: ");
	printuchar(test); printf("\n\n");

	//Run dijkstra

	//Print dijkstra results
	*/

	//Second Setup

	/*Router *root = (Router *)malloc(sizeof(Router));

	//Populate Nodes for Dijkstra test
	Router *a = (Router *)malloc(sizeof(Router));
	Router *b = (Router *)malloc(sizeof(Router));
	Router *c = (Router *)malloc(sizeof(Router));
	Router *d = (Router *)malloc(sizeof(Router));

	a->network[0] = 192;
	a->network[1] = 168;
	a->network[2] = 2;
	a->network[3] = 1;

	a->submask[0] = 255;
	a->submask[1] = 255;
	a->submask[2] = 255;
	a->submask[3] = 0;

	b->network[0] = 192;
	b->network[1] = 168;
	b->network[2] = 3;
	b->network[3] = 1;

	b->submask[0] = 255;
	b->submask[1] = 255;
	b->submask[2] = 255;
	b->submask[3] = 0;

	c->network[0] = 192;
	c->network[1] = 168;
	c->network[2] = 4;
	c->network[3] = 1;

	c->submask[0] = 255;
	c->submask[1] = 255;
	c->submask[2] = 255;
	c->submask[3] = 0;

	d->network[0] = 192;
	d->network[1] = 168;
	d->network[2] = 5;
	d->network[3] = 1;

	d->submask[0] = 255;
	d->submask[1] = 255;
	d->submask[2] = 255;
	d->submask[3] = 0;

	printf("A has network ");printuchar(a->network);printf(" and submask ");printuchar(a->submask);printf("\n");
	
	printf("Comparing gives %d\n:", COMPARE_IP(a->network, a->network));

	//Adding neighbour nodes to a
	add(b,a);
	add(c,a);

	//Adding neightbour nodes to b
	add(a,b);
	add(c,b);
	add(d,b);

	//Adding neightbour nodes to c
	add(a,c);
	add(b,c);
	add(d,c);

	//Adding neightbour nodes to d
	add(b,d);
	add(c,d);


	add(a,root);
	add(b,root);
	add(c,root);
	add(d,root);


	calculateDijkstra(root, a);

	return 0;
}*/