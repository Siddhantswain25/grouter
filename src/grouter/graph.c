/*
 * Operations on the topology graph
 */
#include "graph.h"


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
