//
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
////Predeclared Structs
//typedef struct _Node Node;
//typedef struct _Link Link;
//typedef struct _NextHop NextHop;
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
//void printuchar(uchar ip[]);
//char* getucharstr(uchar ip[]);
//Node* createNode(char *ip, int seq_Numb);
//Link* createLink(char *linkId, char *linkData, int linkType);
//int graphLength(Node *root);
//int neighbourListLength(Node *node);
//void addNextNode(Node **root, Node *node);
//void addNeighbour(Node *node, Link *link);
//void removeAllNeighbours(Node *node);
//void printGraph(Node *node);
//int getLowestDistance(int distance[], int observed[], int numbNodes);
//int calcNextHop(int index, int distance[], int previousNode[]);
////NextHop* calculateDijkstra(Router *root, Router *sourceRouter);
