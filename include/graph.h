#ifndef GRAPH_H_
#define GRAPH_H_

#include <stdlib.h>
#include <stdio.h>
#include "grouter.h"
#include <string.h>

#define TYPE_ANY_TO_ANY		2
#define TYPE_STUB			3
#define INF					99999

//Predeclared Structs
typedef struct _Node Node;
typedef struct _Link Link;
typedef struct _NextHop NextHop;
typedef struct _StubHop StubHop;


//------STRUCTS--------
typedef struct _Link{
	uchar linkId[4];		//-.-.-.0
	uchar linkData[4];		//Netmask
	int linkType;

	struct _Link *next;
}Link;

typedef struct _Node{
	uchar ip[4];			//Equal to network IP

	int seq_Numb;			//Sequence number

	struct _Node *next;				//Next node in graph

	Link *list;				//List of neighbours
}Node;

typedef struct _NextHop{
	uchar rsubmask[4];  //destination subnet
	uchar rnetwork[4]; 	// always 255.255.255.0
	uchar nh_ip[4];		//nexthop (neighbour) ip

	uchar interfaceIp[4];	//interface ip

	NextHop *next;
}NextHop;

typedef struct _StubHop{
	uchar rsubmask[4];
	uchar rnetwork[4];

	int assNode;

	StubHop *next;
}StubHop;

//Functions
void printuchar(uchar ip[]);
char* getucharstr(uchar ip[]);
Node* createNode(char *ip, int seq_Numb);
Link* createLink(char *linkId, char *linkData, int linkType);
int graphLength(Node *root);
int neighbourListLength(Node *node);
void addNextNode(Node **root, Node *node);
void addNeighbour(Node *node, Link *link);
void removeAllNeighbours(Node *node);
void printGraphJeremie(Node *node);
int getNodeWithLowestDistance(int distance[], int observed[], int numbNodes);
int getNodeId(Link *neighbour, Node nodes[], int numbNodes);
int calcNextHop(int index, int distance[], int previousNode[]);
int isNodeAnInterfaceOfRouter(char *ip, uchar interfaces[][4], int numbInterface);
NextHop* calculateDijkstra(Node *head, uchar interfaces[][4], int numbInterface);
void printNextHopList(NextHop *list);
Node *getNodeByIP(Node *graph, uchar ip[]);
void printGraph(Node *graph);
void printNextHops(NextHop *head);
Node *addNode(Node *graph, Node *newNode);
Link *addLink(Link *list, Link *newLink);

#endif /* GRAPH_H_ */
