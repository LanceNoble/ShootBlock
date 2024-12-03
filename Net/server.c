#include "server.h"

#include <winsock2.h>
#include <time.h>
#include <stdio.h>

#include "comms.h"

struct Server {
	unsigned long long udp;
	unsigned short seq;

	unsigned char numOn;
	struct Host clients[MAX_PLAYERS];

	struct Message* msgs;
	struct sockaddr_in* froms;
};

void* server_create(const unsigned short port) {
	struct Server* server = malloc(sizeof(struct Server));
	if (server == NULL) {
		printf("Server Creation Fail: No Memory\n");
		return NULL;
	}

	server->msgs = malloc(sizeof(struct Message) * 255);
	server->froms = malloc(sizeof(struct sockaddr_in) * 255);
	if (server->msgs == NULL || server->froms == NULL) {
		printf("Server Creation Fail: No Memory\n");
		server_destroy(server);
		return NULL;
	}

	server->seq = 1;
	server->numOn = 0;
	for (int i = 0; i < MAX_PLAYERS; i++) {
		server->clients[i].seq = 0;
		server->clients[i].ip = 0;
		server->clients[i].port = 0;
		server->clients[i].time = 0;
		server->clients[i].numMsgs = 0;
		server->clients[i].msgs = malloc(sizeof(struct Message) * 255);
		if (server->clients[i].msgs == NULL) {
			printf("Server Creation Fail: No Memory\n");
			server_destroy(server);
			return NULL;
		}
	}

	server->udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (server->udp == INVALID_SOCKET) {
		printf("Server Creation Fail: Invalid Socket (%u)\n", WSAGetLastError());
		server_destroy(server);
		return NULL;
	}

	unsigned long mode = 1;
	if (ioctlsocket(server->udp, FIONBIO, &mode) == SOCKET_ERROR) {
		printf("Server Creation Fail: Can't No-Block (%u)\n", WSAGetLastError());
		server_destroy(server);
		return NULL;
	}

	struct sockaddr_in binder;
	binder.sin_family = AF_INET;
	binder.sin_addr.S_un.S_addr = INADDR_ANY;
	binder.sin_port = htons(port);
	if (bind(server->udp, (struct sockaddr*)&binder, sizeof(struct sockaddr)) == SOCKET_ERROR) {
		printf("Server Creation Fail: Can't Bind (%u)\n", WSAGetLastError());
		server_destroy(server);
		return NULL;
	}
	
	return server;
}

struct Host* server_sync(void* server) {
	struct Server* cast = server;

	for (int i = 0; i < MAX_PLAYERS; i++) {
		cast->clients[i].numMsgs = 0;
	}
	int numMsgs = 0;
	int fromLen = sizeof(struct sockaddr);

	do {
		cast->msgs[numMsgs].len = recvfrom(cast->udp, cast->msgs[numMsgs].buf, 16, 0, (struct sockaddr*)&(cast->froms[numMsgs]), &fromLen);
		if (cast->msgs[numMsgs].len != SOCKET_ERROR) {
			++numMsgs;
		}
	} while (cast->msgs[numMsgs].len != SOCKET_ERROR && numMsgs < 255);

	for (int i = 0; i < numMsgs; i++) {
		flip(cast->msgs[i].buf, cast->msgs[i].len);
		unsigned short seq = (cast->msgs[i].buf[0] << 8) | cast->msgs[i].buf[1];

		signed short spot = -1;
		unsigned short numLike = 0;
		for (int j = 0; j < MAX_PLAYERS; j++) {
			if (cast->clients[j].ip == 0) {
				spot = j;
			}
			else if (cast->clients[j].ip == cast->froms[i].sin_addr.S_un.S_addr && cast->clients[j].port == cast->froms[i].sin_port) {
				if (seq > cast->clients[j].seq) {
					cast->clients[j].msgs[cast->clients[j].numMsgs++] = cast->msgs[i];
					cast->clients[j].seq = seq;
					cast->clients[j].time = clock();
				}
				numLike++;
			}
		}

		if (spot > -1 && numLike == 0) {
			//printf("Someone joined\n");
			cast->clients[spot].ip = cast->froms[i].sin_addr.S_un.S_addr;
			cast->clients[spot].port = cast->froms[i].sin_port;
			cast->clients[spot].numMsgs = 1;
			cast->clients[spot].msgs[0] = cast->msgs[i];
			cast->clients[spot].seq = seq;
			cast->clients[spot].time = clock();
			cast->numOn++;
		}
	}

	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (cast->clients[i].ip != 0 && (clock() - cast->clients[i].time) / 1000 >= TIMEOUT_HOST) {
			//printf("Empty Spot Detected\n");
			cast->clients[i].ip = 0;
			cast->clients[i].port = 0;
			cast->clients[i].numMsgs = 0;
			cast->clients[i].seq = 0;
			cast->clients[i].time = 0;
		}

		union Response res;
		res.ack = 0;
		res.bit = 0;
		for (int j = 0; j < cast->clients[i].numMsgs; j++) {
			unsigned short seq = (cast->clients[i].msgs[j].buf[0] << 8) | cast->clients[i].msgs[j].buf[1];
			printf("Acknowledging Sequence %i\n", seq);
			//printf("Player move dir: %i\n", cast->clients[i].msgs->buf[0])
			if (j == 0 || seq > res.ack + 16) {
				res.ack = seq;
				res.bit = 0;
				if (j > 0 || cast->clients[i].numMsgs == 1) {
					struct sockaddr_in to;
					to.sin_family = AF_INET;
					to.sin_addr.S_un.S_addr = cast->clients[i].ip;
					to.sin_port = cast->clients[i].port;
					flip(res.raw, sizeof(res));
					sendto(cast->udp, res.raw, sizeof(res), 0, (struct sockaddr*)&to, sizeof(to));
				}
			}
			else {
				res.bit & (1 << ((seq - res.ack) - 1));
			}
		}
	}

	//printf("%p\n", cast->clients);
	//*players = &(cast->clients);
	//printf("%p\n", players);
	return cast->clients;
	//return WSAGetLastError();
}

/*
short server_ping(struct Server* server) {
	union Data data;
	data.pid = PID;
	data.seq = server->seq;
	flip(data.raw, sizeof(union Data));
	for (unsigned char i = 0; i < MAX_PLAYERS; i++) {
		if (server->clients[i].timeRecv > 0) {
			sendto(server->udp, data.raw, sizeof(union Data), 0, (struct sockaddr*)&server->clients[i].addr, sizeof(struct sockaddr));
			printf("Sending Packet %lu\n", server->seq++);
		}
	}
	return 0;
}
*/

void server_destroy(struct Server** server) {
	free(*server);
	*server = NULL;
}