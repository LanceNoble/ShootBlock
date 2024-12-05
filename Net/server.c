#include "server.h"

#include <winsock2.h>
#include <time.h>
#include <stdio.h>

#include "comms.h"

struct Server {
	WSADATA wsa;

	SOCKET udp;
	unsigned short seq;

	unsigned char numOn;
	struct Host clients[MAX_PLAYERS];

	struct Message* msgs;
	struct sockaddr_in* froms;
};

struct Server* server_create(const unsigned short port) {
	struct Server* server = malloc(sizeof(struct Server));
	if (server == NULL) {
		printf("Server Creation Fail: No Memory\n");
		return NULL;
	}

	if (WSAStartup(MAKEWORD(2, 2), &server->wsa) != 0) {
		printf("Server Creation Fail: WSAStartup Fail\n");
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

struct Host* server_sync(struct Server* server) {

	for (int i = 0; i < MAX_PLAYERS; i++) {
		server->clients[i].numMsgs = 0;
	}
	int numMsgs = 0;
	int fromLen = sizeof(struct sockaddr);

	do {
		server->msgs[numMsgs].len = recvfrom(server->udp, server->msgs[numMsgs].buf, 16, 0, (struct sockaddr*)&(server->froms[numMsgs]), &fromLen);
		if (server->msgs[numMsgs].len != SOCKET_ERROR) {
			++numMsgs;
		}
	} while (server->msgs[numMsgs].len != SOCKET_ERROR && numMsgs < 255);

	for (int i = 0; i < numMsgs; i++) {
		unsigned short seq = (server->msgs[i].buf[0] << 8) | server->msgs[i].buf[1];

		signed short spot = -1;
		unsigned short numLike = 0;
		for (int j = 0; j < MAX_PLAYERS; j++) {
			if (server->clients[j].ip == 0) {
				spot = j;
			}
			else if (server->clients[j].ip == server->froms[i].sin_addr.S_un.S_addr && server->clients[j].port == server->froms[i].sin_port) {
				if (seq > server->clients[j].seq) {
					server->clients[j].msgs[server->clients[j].numMsgs++] = server->msgs[i];
					server->clients[j].seq = seq;
					server->clients[j].time = clock();
				}
				numLike++;
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

	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (server->clients[i].ip != 0 && (clock() - server->clients[i].time) / CLOCKS_PER_SEC >= TIMEOUT_HOST) {
			server->clients[i].ip = 0;
			server->clients[i].port = 0;
			server->clients[i].numMsgs = 0;
			server->clients[i].seq = 0;
			server->clients[i].time = 0;
		}

		union Response res;
		res.ack = 0;
		res.bit = 0;
		for (int j = 0; j < server->clients[i].numMsgs; j++) {
			unsigned short seq = (server->clients[i].msgs[j].buf[0] << 8) | server->clients[i].msgs[j].buf[1];
			if (j == 0 || seq > res.ack + 16) {
				res.ack = seq;
				res.bit = 0;
				if (j > 0 || server->clients[i].numMsgs == 1) {
					struct sockaddr_in to;
					to.sin_family = AF_INET;
					to.sin_addr.S_un.S_addr = server->clients[i].ip;
					to.sin_port = server->clients[i].port;
					for (int i = 0; i < 17; i++) {
						sendto(server->udp, res.raw, sizeof(res), 0, (struct sockaddr*)&to, sizeof(to));
					}
				}
			}
			else {
				res.bit & (1 << (seq - res.ack - 1));
			}
		}
	}

	//printf("%p\n", cast->clients);
	//*players = &(cast->clients);
	//printf("%p\n", players);
	return server->clients;
	//return WSAGetLastError();
}

void server_ping(struct Server* server, struct Message state) {

	unsigned char sendBuf[32];
	sendBuf[0] = (server->seq & (0xff << 8)) >> 8;
	sendBuf[1] = server->seq & 0xff;
	for (int i = 0, j = 2; i < state.len; i++, j++) {
		sendBuf[j] = state.buf[i];
	}

	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (server->clients[i].ip != 0) {
			struct sockaddr_in to;
			to.sin_family = AF_INET;
			to.sin_addr.S_un.S_addr = server->clients[i].ip;
			to.sin_port = server->clients[i].port;

			
			for (int i = 0; i < 17; i++) {
				sendto(server->udp, sendBuf, state.len + 2, 0, (struct sockaddr*)&to, sizeof(struct sockaddr));
			}
			server->seq++;
		}
	}
}

void server_destroy(struct Server* server) {
	free(server);
	//*server = NULL;
}