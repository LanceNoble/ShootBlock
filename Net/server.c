#include "server.h"

#include <winsock2.h>
#include <time.h>
#include <stdio.h>

#include "comms.h"

struct Host {
	struct sockaddr_in addr;
	clock_t time;
	unsigned short seq;
};

struct Server {
	WSADATA wsa;
	SOCKET udp;
	unsigned short seq;

	struct Host client1;
	struct Host client2;
};

struct Server* server_create(const unsigned short port) {
	struct Server* server = malloc(sizeof(struct Server));
	if (server == NULL || WSAStartup(MAKEWORD(2, 2), &server->wsa) != 0) {
		return NULL;
	}
	server->seq = 1;

	server->client1.addr.sin_family = AF_INET;
	server->client1.addr.sin_addr.S_un.S_addr = INADDR_ANY;
	server->client1.addr.sin_port = 0;
	server->client1.time = 0;
	server->client1.seq = 0;

	server->client2.addr.sin_family = AF_INET;
	server->client2.addr.sin_addr.S_un.S_addr = INADDR_ANY;
	server->client2.addr.sin_port = 0;
	server->client2.time = 0;
	server->client2.seq = 0;

	server->udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (server->udp == INVALID_SOCKET) {
		server_destroy(server);
		return NULL;
	}

	u_long mode = 1;
	if (ioctlsocket(server->udp, FIONBIO, &mode) == SOCKET_ERROR) {
		server_destroy(server);
		return NULL;
	}

	struct sockaddr_in binder;
	binder.sin_family = AF_INET;
	binder.sin_addr.S_un.S_addr = INADDR_ANY;
	binder.sin_port = htons(port);
	if (bind(server->udp, (struct sockaddr*)&binder, sizeof(struct sockaddr)) == SOCKET_ERROR) {
		server_destroy(server);
		return NULL;
	}
	
	return server;
}

void server_sync(struct Server* server, char* inputs) {
	char* ptr = inputs;
	char* len;
	struct Host* sender = NULL;
	struct Host* introvert = NULL;
	struct sockaddr_in from;
	int fromlen = sizeof(struct sockaddr);
	int numMsgs = 0;

	while (1) {
		len = ptr++;
		*len = recvfrom(server->udp, ptr, 8, 0, (struct sockaddr*)&from, &fromlen);
		if (*len == SOCKET_ERROR || numMsgs == 16) {
			break;
		}
		//printf("Y");
		ptr += *len;

		if (from.sin_addr.S_un.S_addr == server->client1.addr.sin_addr.S_un.S_addr && from.sin_port == server->client1.addr.sin_port) {
			sender = &server->client1;
			introvert = &server->client2;
		}
		else if (from.sin_addr.S_un.S_addr == server->client2.addr.sin_addr.S_un.S_addr && from.sin_port == server->client2.addr.sin_port) {
			sender = &server->client2;
			introvert = &server->client1;
			*len |= 0b10000000;
		}
		else if (server->client1.addr.sin_addr.S_un.S_addr == 0) {
			server->client1.addr.sin_addr.S_un.S_addr = from.sin_addr.S_un.S_addr;
			server->client1.addr.sin_port = from.sin_port;
			sender = &server->client1;
			introvert = &server->client2;
		}
		else if (server->client2.addr.sin_addr.S_un.S_addr == 0) {
			server->client2.addr.sin_addr.S_un.S_addr = from.sin_addr.S_un.S_addr;
			server->client2.addr.sin_port = from.sin_port;
			sender = &server->client2;
			introvert = &server->client1;
			*len |= 0b10000000;
		}

		unsigned short seq = (len[1] << 8) | len[2];
		if (sender != NULL && seq > sender->seq) {
			sender->seq = seq;
			sender->time = clock();
		}

		if (introvert != NULL && introvert->addr.sin_addr.S_un.S_addr != 0 && (clock() - introvert->time) / CLOCKS_PER_SEC >= TIMEOUT_HOST) {
			introvert->addr.sin_addr.S_un.S_addr = 0;
			introvert->addr.sin_port = 0;
			introvert->time = 0;
			introvert->seq = 0;
		}

		numMsgs++;
	}
	*len = '\0';

	/*
	int numClient1Seqs = 0;
	int numClient2Seqs = 0;
	unsigned short client1Seqs[16];
	unsigned short client2Seqs[16];

	for (int i = 0; i < numMsgs; i++) {
		unsigned short seq = (inputs[1] << 8) | inputs[2];
		if (*inputs & 0b10000000) {
			client2Seqs[numClient2Seqs++] = seq;
		}
		else {
			client1Seqs[numClient1Seqs++] = seq;
		}
		inputs += (*inputs & 0b01111111) + 1;
	}

	union Response res;
	for (int i = 0; i < numClient1Seqs;) {
		res.ack = client1Seqs[i];
		res.bit = 0;
		while (++i < numClient1Seqs && client1Seqs[i] <= res.ack + 16) {
			res.bit &= (1 << (client1Seqs[i] - res.ack - 1));
		}
		for (int i = 0; i < 17; i++) {
			sendto(server->udp, res.raw, sizeof(res), 0, (struct sockaddr*)&server->client1.addr, sizeof(struct sockaddr));
		}
	}

	for (int i = 0; i < numClient2Seqs; i++) {
		res.ack = client2Seqs[i];
		res.bit = 0;
		while (++i < numClient2Seqs && client2Seqs[i] <= res.ack + 16) {
			res.bit &= (1 << (client2Seqs[i] - res.ack - 1));
		}
		for (int i = 0; i < 17; i++) {
			sendto(server->udp, res.raw, sizeof(res), 0, (struct sockaddr*)&server->client2.addr, sizeof(struct sockaddr));
		}
	}
	*/
}

/*
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

			sendto(server->udp, sendBuf, state.len + 2, 0, (struct sockaddr*)&to, sizeof(struct sockaddr));
			//for (int i = 0; i < 17; i++) {
			//	sendto(server->udp, sendBuf, state.len + 2, 0, (struct sockaddr*)&to, sizeof(struct sockaddr));
			//}
			server->seq++;
		}
	}
}
*/

void server_destroy(struct Server* server) {
	free(server);
}