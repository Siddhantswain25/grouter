///*
//* dijkstra.h --> header file for Dijkstra algorithm
//*
//* author --> Jeremie
//*/
//
//#include <stdlib.h>
//#include <stdio.h>
//#include "grouter.h"
//
//#define INFINITY	99999
//
////------STRUCTS--------
//typedef struct _Link{
//	uchar linkId[4];		//-.-.-.0
//	uchar linkData[4];		//Netmask
//	int linkType;
//
//	Link *next;
//}Link;
//
//typedef struct _Node{
//	uchar ip[4];			//Equal to network IP
//
//	int seq_Numb;			//Sequence number
//
//	Node *next;				//Next node in graph
//
//	Link *list;				//List of neighbours
//}Node;
//
//typedef struct _NextHop{
//	uchar rsubmask[4];
//	uchar rnetwork[4];
//	uchar nh_ip[4];
//	NextHop *next;
//}NextHop;
//
//
//
//
////Predeclared Functions
///*void printList(Node *list);
//void printuchar(uchar ip[]);
//char* getucharstr(uchar ip[]);
//int length(Node *givenList);
//void add(Router *rToAdd, Router *addToThisR);
//int getLowestDistance(int distance[], int observed[], int numbNodes);
//int getNodeId(Router *router, Router routers[], int numbNodes);
//int calcNextHop(int index, int distance[], int previousNode[]);
//NextHop* calculateDijkstra(Router *root, Router *sourceRouter);
//int test();*/
