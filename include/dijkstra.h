/*
* dijkstra.h --> header file for Dijkstra algorithm
*
* author --> Jeremie
*/

#include <stdlib.h>
#include <stdio.h>
#include "grouter.h"

#define INFINITY	99999

//Predeclared Structs
typedef struct _Router Router;
typedef struct _Node Node;
typedef struct _NextHop NextHop;

//------STRUCTS--------
typedef struct _Node{
	Router *router;
	Node *next;
}Node;

typedef struct _Router{
	uchar submask[4];
	uchar network[4];
	Node *list;
}Router;

typedef struct _NextHop{
	uchar rsubmask[4];
	uchar rnetwork[4];
	uchar nh_ip[4];
	NextHop *next;
}NextHop;




//Predeclared Functions
void printList(Node *list);
void printuchar(uchar ip[]);
char* getucharstr(uchar ip[]);
int length(Node *givenList);
void add(Router *rToAdd, Router *addToThisR);
int getLowestDistance(int distance[], int observed[], int numbNodes);
int getNodeId(Router *router, Router routers[], int numbNodes);
int calcNextHop(int index, int distance[], int previousNode[]);
NextHop* calculateDijkstra(Router *root, Router *sourceRouter);
int test();
