#include "server.h"

#include <winsock2.h>
#include <time.h>
#include <stdio.h>

#include "comms.h"

struct Host {
	struct sockaddr_in addr;
	clock_t time;
	unsigned short seq;

	int numSeqs;
	unsigned short seqs[16];
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
		server_destroy(server);
		return NULL;
	}

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
	if (bind(server->udp, (struct sockaddr*)&binder, sizeof(binder)) == SOCKET_ERROR) {
		server_destroy(server);
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
	
	return server;
}

void server_sync(struct Server* server, unsigned char* buf) {
	unsigned char* i = buf;
	unsigned char* meta; // unsigned since we control the first bit to represent who sent the message
	int numMsgs = 0;

	struct Host* sender;
	struct sockaddr_in from;
	int fromlen = sizeof(struct sockaddr);

	server->client1.numSeqs = 0;
	server->client2.numSeqs = 0;

	do {
		sender = NULL;

		meta = i++;
		*meta = recvfrom(server->udp, i, 8, 0, (struct sockaddr*)&from, &fromlen);
		i += *meta;

		if (*meta != (unsigned char)SOCKET_ERROR && from.sin_addr.S_un.S_addr == server->client1.addr.sin_addr.S_un.S_addr && from.sin_port == server->client1.addr.sin_port) {
			sender = &server->client1;
		}
		else if (*meta != (unsigned char)SOCKET_ERROR && from.sin_addr.S_un.S_addr == server->client2.addr.sin_addr.S_un.S_addr && from.sin_port == server->client2.addr.sin_port) {
			sender = &server->client2;
			*meta |= 0b10000000;
		}
		else if (*meta != (unsigned char)SOCKET_ERROR && server->client1.addr.sin_addr.S_un.S_addr == 0) {
			server->client1.addr.sin_addr.S_un.S_addr = from.sin_addr.S_un.S_addr;
			server->client1.addr.sin_port = from.sin_port;
			sender = &server->client1;
		}
		else if (*meta != (unsigned char)SOCKET_ERROR && server->client2.addr.sin_addr.S_un.S_addr == 0) {
			server->client2.addr.sin_addr.S_un.S_addr = from.sin_addr.S_un.S_addr;
			server->client2.addr.sin_port = from.sin_port;
			sender = &server->client2;
			*meta |= 0b10000000;
		}

		unsigned short seq = (meta[1] << 8) | meta[2];
		if (sender != NULL && seq > sender->seq) {
			sender->seq = seq;
			sender->time = clock();
			sender->seqs[sender->numSeqs++] = seq;
			numMsgs++;
		}
		else {
			i = meta;
		}

		if ((clock() - server->client1.time) / CLOCKS_PER_SEC >= TIMEOUT_HOST) {
			server->client1.addr.sin_addr.S_un.S_addr = 0;
			server->client1.addr.sin_port = 0;
			server->client1.time = 0;
			server->client1.seq = 0;
		}

		if ((clock() - server->client2.time) / CLOCKS_PER_SEC >= TIMEOUT_HOST) {
			server->client2.addr.sin_addr.S_un.S_addr = 0;
			server->client2.addr.sin_port = 0;
			server->client2.time = 0;
			server->client2.seq = 0;
		}

		printf("%i\n", numMsgs);
	} while (*meta != (unsigned char)SOCKET_ERROR && numMsgs < 16);
	if (numMsgs == 16) {
		meta = i;
	}
	*meta = '\0';

	union Response res;
	for (int i = 0; i < server->client1.numSeqs;) {
		res.ack = server->client1.seqs[i];
		res.bit = 0;
		while (++i < server->client1.numSeqs && server->client1.seqs[i] <= res.ack + 16) {
			res.bit &= (1 << (server->client1.seqs[i] - res.ack - 1));
		}
		for (int i = 0; i < 17; i++) {
			sendto(server->udp, res.raw, sizeof(res), 0, (struct sockaddr*)&server->client1.addr, sizeof(struct sockaddr));
		}
	}

	for (int i = 0; i < server->client2.numSeqs;) {
		res.ack = server->client2.seqs[i];
		res.bit = 0;
		while (++i < server->client2.numSeqs && server->client2.seqs[i] <= res.ack + 16) {
			res.bit &= (1 << (server->client2.seqs[i] - res.ack - 1));
		}
		for (int i = 0; i < 17; i++) {
			sendto(server->udp, res.raw, sizeof(res), 0, (struct sockaddr*)&server->client2.addr, sizeof(struct sockaddr));
		}
	}
}

void server_ping(struct Server* server, unsigned char* buf) {
	buf[0] = (server->seq & (0xff << 8)) >> 8;
	buf[1] = server->seq & 0xff;

	for (int i = 0; i < 17; i++) {
		sendto(server->udp, buf, 18, 0, (struct sockaddr*)&server->client1.addr, sizeof(struct sockaddr));
	}
	for (int i = 0; i < 17; i++) {
		sendto(server->udp, buf, 18, 0, (struct sockaddr*)&server->client2.addr, sizeof(struct sockaddr));
	}

	server->seq++;
	//printf("%i\n", server->seq);
}

void server_destroy(struct Server* server) {
	free(server);
}