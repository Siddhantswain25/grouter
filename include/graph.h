#ifndef GRAPH_H_
#define GRAPH_H_

#include <stdlib.h>
#include <stdio.h>
#include "grouter.h"

#define TYPE_ANY_TO_ANY		2
#define TYPE_STUB			3

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

Node *getNodeByIP(Node *graph, uchar ip[]);
void printGraph(Node *graph);
Node *addNode(Node *graph, Node *newNode);
Link *addLink(Link *list, Link *newLink);

#endif /* GRAPH_H_ */
