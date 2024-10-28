#include "server.h"

#include <winsock2.h>
#include <time.h>
#include <stdio.h>

#include "comms.h"

#define PLAYERMAX 2

struct Spot {
	unsigned long val;
	struct Spot* next;
};

struct Host {
	struct sockaddr_in addr;
	unsigned long elapse;
};

struct Server {
	SOCKET udp;
	struct Host players[PLAYERMAX];
	unsigned char numOn;
	struct Spot* firstOpen;
	struct Spot* lastOpen;
};

struct Server* server_create(unsigned short port) {
	struct Server* server = malloc(sizeof(struct Server));
	if (server == NULL)
		return NULL;

	server->numOn = 0;
	server->firstOpen = malloc(sizeof(struct Spot));
	if (server->firstOpen == NULL) {
		server_destroy(&server);
		return NULL;
	}
	server->lastOpen = server->firstOpen;
	for (unsigned char i = 0; i < PLAYERMAX; i++) {
		server->players[i].elapse = 0;

		server->lastOpen->val = i;
		if (i == PLAYERMAX - 1) {
			server->lastOpen->next = NULL;
			break;
		}

		server->lastOpen->next = malloc(sizeof(struct Spot));
		
		if (server->lastOpen->next == NULL) {
			server_destroy(&server);
			return NULL;
		}
		server->lastOpen = server->lastOpen->next;
	}

	server->udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	struct sockaddr_in addrBind;
	addrBind.sin_family = AF_INET;
	addrBind.sin_addr.S_un.S_addr = INADDR_ANY;
	addrBind.sin_port = htons(port);

	if (bind(server->udp, (struct sockaddr*)&addrBind, sizeof(struct sockaddr)) == SOCKET_ERROR) {
		printf("failed to bind socket\n");
		server_destroy(&server);
		return NULL;
	}
	
	unsigned long blockMode = 1;
	ioctlsocket(server->udp, FIONBIO, &blockMode);

	return server;
}

void server_sync(struct Server* server) {
	if (server == NULL) {
		return;
	}

	for (unsigned char i = 0; i < PLAYERMAX; i++) {
		if (server->players[i].elapse == 0) {
			continue;
		}
		if ((clock() - server->players[i].elapse) / (double)CLOCKS_PER_SEC >= TIMEOUT) {
			printf("Player left\n");
			server->players[i].elapse = 0;
			struct Spot* temp = malloc(sizeof(struct Spot));
			if (temp == NULL) {
				server_destroy(&server);
				return;
			}
			temp->val = i;
			temp->next = NULL;
			if (server->firstOpen == NULL) {
				server->firstOpen = temp;
				server->lastOpen = server->firstOpen;
			}
			else if (server->lastOpen != NULL) {
				server->lastOpen->next = temp;
			}
			server->numOn--;
		}
	}

	union Data from;
	struct sockaddr_in fromAddr;
	unsigned long fromAddrLen = sizeof(struct sockaddr);
	short numBytes = recvfrom(server->udp, from.raw, (unsigned long)sizeof(union Data), 0, (struct sockaddr*)&fromAddr, &fromAddrLen);
	if (numBytes == SOCKET_ERROR) {
		numBytes = WSAGetLastError();
		return;
	}

	flip(from.raw, sizeof(union Data));
	if (from.pid != PID) {
		return;
	}

	unsigned char i = 0;
	while (i < PLAYERMAX) {
		if (server->players[i].addr.sin_addr.S_un.S_addr == fromAddr.sin_addr.S_un.S_addr && 
			server->players[i].addr.sin_port == fromAddr.sin_port &&
			server->players[i].elapse != 0) {
			break;
		}
		i++;
	}
	if (i == PLAYERMAX && server->numOn < PLAYERMAX) {
		printf("Player joined\n");
		server->players[server->firstOpen->val].addr = fromAddr;
		server->players[server->firstOpen->val].elapse = clock();

		struct Spot* recentOccupation = server->firstOpen;
		server->firstOpen = server->firstOpen->next;
		free(recentOccupation);
		
		server->numOn++;
		return;
	}

	if (i == PLAYERMAX) {
		return;
	}

	server->players[i].elapse = clock();
}

void server_ping(struct Server* server) {
	if (server == NULL) {
		return;
	}

	union Data data;
	data.pid = PID;
	flip(data.raw, sizeof(union Data));
	for (unsigned char i = 0; i < PLAYERMAX; i++) {
		if (server->players[i].elapse == 0) {
			continue;
		}
		sendto(server->udp, data.raw, sizeof(union Data), 0, (struct sockaddr*)&server->players[i].addr, sizeof(struct sockaddr));
	}
}

static void spots_destroy(struct Spot** head) {
	if (*head == NULL) {
		return;
	}
	spots_destroy(&((*head)->next));
	free(*head);
	*head = NULL;
}

void server_destroy(struct Server** server) {
	spots_destroy(&((*server)->firstOpen));
	free(*server);
	*server = NULL;
}
