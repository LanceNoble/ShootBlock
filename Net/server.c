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
	unsigned long long udp;
	unsigned char numOn;
	struct Host players[PLAYERMAX];
	struct Spot* firstOpen;
	struct Spot* lastOpen;
	unsigned long seq;
};

short server_create(unsigned short port, struct Server** server) {
	*server = malloc(sizeof(struct Server));
	if (*server == NULL) {
		return -1;
	}

	(*server)->numOn = 0;
	(*server)->seq = 0;

	(*server)->firstOpen = malloc(sizeof(struct Spot));
	if ((*server)->firstOpen == NULL) {
		return -2;
	}
	(*server)->lastOpen = (*server)->firstOpen;
	for (unsigned char i = 0; i < PLAYERMAX; i++) {
		(*server)->players[i].elapse = 0;
		(*server)->lastOpen->val = i;

		// If this is the last iteration, the tail ends here
		if (i == PLAYERMAX - 1) {
			(*server)->lastOpen->next = NULL;
			break;
		}

		(*server)->lastOpen->next = malloc(sizeof(struct Spot));
		if ((*server)->lastOpen->next == NULL) {
			return -3;
		}
		(*server)->lastOpen = (*server)->lastOpen->next;
	}

	(*server)->udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if ((*server)->udp == INVALID_SOCKET) {
		return WSAGetLastError();
	}

	unsigned long mode = 1;
	if (ioctlsocket((*server)->udp, FIONBIO, &mode) == SOCKET_ERROR) {
		return WSAGetLastError();
	}

	struct sockaddr_in binder;
	binder.sin_family = AF_INET;
	binder.sin_addr.S_un.S_addr = INADDR_ANY;
	binder.sin_port = htons(port);

	if (bind((*server)->udp, (struct sockaddr*)&binder, sizeof(struct sockaddr)) == SOCKET_ERROR) {
		return WSAGetLastError();
	}
	
	return 0;
}

short server_sync(struct Server* server) {
	for (unsigned char i = 0; i < PLAYERMAX; i++) {
		if (server->players[i].elapse == 0 || (clock() - server->players[i].elapse) / (double)CLOCKS_PER_SEC < TIMEOUT) {
			continue;
		}
		struct Spot* temp = malloc(sizeof(struct Spot));
		if (temp == NULL) {
			continue;
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
		server->players[i].elapse = 0;
		
		printf("Player %lu.%lu.%lu.%lu:%lu Left\n",
			server->players[i].addr.sin_addr.S_un.S_un_b.s_b1,
			server->players[i].addr.sin_addr.S_un.S_un_b.s_b2,
			server->players[i].addr.sin_addr.S_un.S_un_b.s_b3,
			server->players[i].addr.sin_addr.S_un.S_un_b.s_b4,
			server->players[i].addr.sin_port);
	}

	union Data from;
	struct sockaddr_in fromAddr;
	unsigned long fromAddrLen = sizeof(struct sockaddr);
	short numRecv = recvfrom(server->udp, from.raw, (unsigned long)sizeof(union Data), 0, (struct sockaddr*)&fromAddr, &fromAddrLen);
	if (numRecv == SOCKET_ERROR) {
		return WSAGetLastError();
	}

	flip(from.raw, sizeof(union Data));
	if (from.pid != PID) {
		return -1;
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
		server->players[server->firstOpen->val].addr = fromAddr;
		server->players[server->firstOpen->val].elapse = clock();

		struct Spot* recentOccupation = server->firstOpen;
		server->firstOpen = server->firstOpen->next;
		free(recentOccupation);
		
		server->numOn++;

		printf("Player %lu.%lu.%lu.%lu:%lu Joined\n", 
			fromAddr.sin_addr.S_un.S_un_b.s_b1,
			fromAddr.sin_addr.S_un.S_un_b.s_b2,
			fromAddr.sin_addr.S_un.S_un_b.s_b3,
			fromAddr.sin_addr.S_un.S_un_b.s_b4,
			fromAddr.sin_port);

		return -2;
	}

	if (i == PLAYERMAX) {
		return -3;
	}

	server->players[i].elapse = clock();
	return 0;
}

short server_ping(struct Server* server) {
	union Data data;
	data.pid = PID;
	data.seq = server->seq;
	flip(data.raw, sizeof(union Data));
	for (unsigned char i = 0; i < PLAYERMAX; i++) {
		if (server->players[i].elapse > 0) {
			sendto(server->udp, data.raw, sizeof(union Data), 0, (struct sockaddr*)&server->players[i].addr, sizeof(struct sockaddr));
			printf("Sending Packet %lu\n", server->seq++);
		}
	}
	return 0;
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
