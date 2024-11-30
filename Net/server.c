#include "server.h"

#include <winsock2.h>
#include <time.h>
#include <stdio.h>

#include "comms.h"

#define MAX_PLAYERS 2

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
	for (unsigned char i = 0; i < MAX_PLAYERS; i++) {
		server->clients[i].seq = 0;
		server->clients[i].ip = 0;
		server->clients[i].port = 0;
		server->clients[i].time = 0;
		server->clients[i].numMsgs = 0;
		server->clients[i].msgs = malloc(sizeof(struct Message) * 255);
		if (server->clients[i].msgs == NULL) {
			printf("Server Creation Fail: No Memory\n");
			server_destroy(server);
			return 1;
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

/*
unsigned short server_sync(void* server, char** ins, unsigned char* lens) {
	struct Server* cast = server;

	for (unsigned char i = 0; i < MAX_PLAYERS; i++) {
		server->clients[i].numMsgs = 0;
	}
	unsigned char numMsgs = 0;
	unsigned char fromLen = sizeof(struct sockaddr);

	do {
		server->froms[numMsgs].sin_addr.S_un.S_addr = 0;
		server->msgs[numMsgs].len = recvfrom(server->udp, server->msgs[numMsgs].buf, 255, 0, (struct sockaddr*)&(server->froms[numMsgs]), &fromLen);
		if (server->froms[numMsgs].sin_addr.S_un.S_addr != 0) {
			++numMsgs;
		}
	} while (server->froms[numMsgs].sin_addr.S_un.S_addr != 0 && numMsgs < 255);

	for (unsigned char i = 0; i < numMsgs; i++) {
		flip(server->msgs[i].buf, server->msgs[i].len);
		unsigned short seq = (server->msgs[i].buf[0] << 8) | server->msgs[i].buf[1];

		signed char spot = -1;
		unsigned char numLike = 0;
		for (unsigned char j = 0; j < MAX_PLAYERS; j++) {
			if (server->clients[j].ip == server->froms[i].sin_addr.S_un.S_addr && server->clients[j].port == server->froms[i].sin_port) {
				if (seq > server->clients[j].seq) {
					server->clients[j].msgs[server->clients[j].numMsgs++] = server->msgs[i];
					server->clients[j].seq = seq;
					server->clients[j].time = clock();
				}
				numLike++;
			}
			if ((clock() - server->clients[j].time) / 1000 >= TIMEOUT_HOST) {
				server->clients[j].ip = 0;
				server->clients[j].port = 0;
				server->clients[j].numMsgs = 0;
				server->clients[j].seq = 0;
				server->clients[j].time = 0;
				spot = j;
			}
		}

		if (spot > -1 && numLike == 0) {
			server->clients[spot].ip = server->froms[i].sin_addr.S_un.S_addr;
			server->clients[spot].port = server->froms[i].sin_port;
			server->clients[spot].numMsgs = 1;
			server->clients[spot].msgs[0] = server->msgs[i];
			server->clients[spot].seq = seq;
			server->clients[spot].time = clock();
			server->numOn++;
		}
	}

	for (unsigned char i = 0; i < MAX_PLAYERS; i++) {
		union Response res;
		for (unsigned char j = 0; j < server->clients[i].numMsgs; j++) {
			unsigned short seq = (server->clients[i].msgs[j].buf[0] << 8) | server->clients[i].msgs[j].buf[1];
			if (j == 0 || seq > res.ack + 16) {
				if (j > 0) {
					struct sockaddr_in to;
					to.sin_family = AF_INET;
					to.sin_addr.S_un.S_addr = server->clients[i].ip;
					to.sin_port = server->clients[i].port;
					sendto(server->udp, res.raw, sizeof(res), 0, (struct sockaddr*)&to, sizeof(to));
				}
				res.ack = (server->clients[i].msgs[j].buf[0] << 8) | server->clients[i].msgs[j].buf[1];
				res.bit = 0;
			}
			else {
				res.bit & (1 << ((seq - res.ack) - 1));
			}
		}
	}
	
	return WSAGetLastError();
}

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