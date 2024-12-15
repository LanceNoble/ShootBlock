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
	unsigned short seqs[512];
};

struct Server {
	WSADATA wsa;
	unsigned short seq;

	SOCKET udp;
	struct pollfd pfds[3];
	
	struct Host client1;
	struct Host client2;
};

struct Server* server_create(const unsigned short port) {
	struct Server* server = malloc(sizeof(struct Server));

	if (server == NULL || WSAStartup(MAKEWORD(2, 2), &server->wsa) != 0) {
		server_destroy(server);
		return NULL;
	}

	struct sockaddr_in binder;
	binder.sin_family = AF_INET;
	binder.sin_addr.S_un.S_addr = INADDR_ANY;
	binder.sin_port = htons(port);

	u_long blockMode = 1;
	server->udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	server->pfds[0].fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	server->pfds[0].events = POLLIN;
	server->pfds[1].fd = INVALID_SOCKET;
	server->pfds[1].events = POLLHUP | POLLIN;
	server->pfds[2].fd = INVALID_SOCKET;
	server->pfds[2].events = POLLHUP | POLLIN;

	if (server->udp == INVALID_SOCKET || 
		server->pfds[0].fd == INVALID_SOCKET ||
		ioctlsocket(server->udp, FIONBIO, &blockMode) == SOCKET_ERROR || 
		ioctlsocket(server->pfds[0].fd, FIONBIO, &blockMode) == SOCKET_ERROR ||
		bind(server->udp, (struct sockaddr*)&binder, sizeof(binder)) == SOCKET_ERROR ||
		bind(server->pfds[0].fd, (struct sockaddr*)&binder, sizeof(binder)) == SOCKET_ERROR ||
		listen(server->pfds[0].fd, 2) == SOCKET_ERROR) {

		server_destroy(server);
		return NULL;
	}
	
	server->seq = 0;

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

void server_meet(struct Server* server) {
	WSAPoll(server->pfds, 3, 0);
	
	if (server->pfds[0].revents & POLLIN) {
		unsigned char intro[1];

		struct sockaddr_in addr;
		int addrlen = sizeof(struct sockaddr);

		SOCKET guest = accept(server->pfds[0].fd, (struct sockaddr*)&addr, &addrlen);

		if (addr.sin_addr.S_un.S_addr == server->client1.addr.sin_addr.S_un.S_addr && addr.sin_port == server->client1.addr.sin_port) {
			server->client1.addr.sin_addr.S_un.S_addr = 0;
			server->client1.addr.sin_port = 0;
			server->client1.time = 0;
			server->client1.seq = 0;
			closesocket(guest);
		}
		else if (addr.sin_addr.S_un.S_addr == server->client2.addr.sin_addr.S_un.S_addr && addr.sin_port == server->client2.addr.sin_port) {
			server->client2.addr.sin_addr.S_un.S_addr = 0;
			server->client2.addr.sin_port = 0;
			server->client2.time = 0;
			server->client2.seq = 0;
			closesocket(guest);
		}
		else if (server->client1.addr.sin_addr.S_un.S_addr == 0) {
			server->client1.addr.sin_addr.S_un.S_addr = addr.sin_addr.S_un.S_addr;
			server->client1.addr.sin_port = addr.sin_port;
			server->client1.time = clock();
			intro[0] = 1;
			send(guest, intro, 1, 0);
		}
		else if (server->client2.addr.sin_addr.S_un.S_addr == 0) {
			server->client2.addr.sin_addr.S_un.S_addr = addr.sin_addr.S_un.S_addr;
			server->client2.addr.sin_port = addr.sin_port;
			server->client2.time = clock();
			intro[0] = 2;
			send(guest, intro, 1, 0);
		}
		else {
			closesocket(guest);
		}
	}

	if (server->pfds[1].revents & POLLHUP || server->pfds[1].revents & POLLIN) {
		closesocket(server->pfds[1].fd);
		server->pfds[1].fd == INVALID_SOCKET;
	}

	if (server->pfds[2].revents & POLLHUP || server->pfds[1].revents & POLLIN) {
		closesocket(server->pfds[2].fd);
		server->pfds[2].fd == INVALID_SOCKET;
	}
}

void server_sync(struct Server* server, unsigned char* buf) {
	unsigned char* iBuf = buf;
	unsigned char* meta;

	struct sockaddr_in* from;
	int fromlen = sizeof(struct sockaddr);

	server->client1.numSeqs = 0;
	server->client2.numSeqs = 0;

	do {
		meta = iBuf++;
		*meta = recvfrom(server->udp, iBuf + fromlen, 8, 0, (struct sockaddr*)iBuf, &fromlen);
		iBuf += *meta + fromlen;
		from = meta + 1;

		unsigned short seq = (meta[17] << 8) | meta[18];
		
		if (*meta != (unsigned char)SOCKET_ERROR && from->sin_addr.S_un.S_addr == server->client1.addr.sin_addr.S_un.S_addr && from->sin_port == server->client1.addr.sin_port && seq > server->client1.seq) {
			server->client1.time = clock();
			server->client1.seq = seq;
			server->client1.seqs[server->client1.numSeqs++] = seq;
		}
		else if (*meta != (unsigned char)SOCKET_ERROR & from->sin_addr.S_un.S_addr == server->client2.addr.sin_addr.S_un.S_addr && from->sin_port == server->client2.addr.sin_port && seq > server->client2.seq) {
			server->client2.time = clock();
			server->client2.seq = seq;
			server->client2.seqs[server->client2.numSeqs++] = seq;
		}
		else {
			iBuf = meta;
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
	} while (*meta != (unsigned char)SOCKET_ERROR);

	*meta = '\0';
	union Response res;
	
	for (int i = 0; i < server->client1.numSeqs;) {
		res.ack = server->client1.seqs[i];
		res.bit = 0;

		while (++i < server->client1.numSeqs && server->client1.seqs[i] < res.ack + 17) {
			res.bit |= (1 << (server->client1.seqs[i] - res.ack - 1));
		}

		for (int i = 0; i < 17; i++) {
			sendto(server->udp, res.raw, sizeof(res), 0, (struct sockaddr*)&server->client1.addr, sizeof(struct sockaddr));
		}
	}

	for (int i = 0; i < server->client2.numSeqs;) {
		res.ack = server->client2.seqs[i];
		res.bit = 0;
		
		while (++i < server->client2.numSeqs && server->client2.seqs[i] < res.ack + 17) {
			res.bit |= (1 << (server->client2.seqs[i] - res.ack - 1));
		}
		
		for (int i = 0; i < 17; i++) {
			sendto(server->udp, res.raw, sizeof(res), 0, (struct sockaddr*)&server->client2.addr, sizeof(struct sockaddr));
		}
	}
}

void server_ping(struct Server* server, unsigned char* buf) {
	server->seq++;

	buf[0] = 0;
	buf[1] = 0;
	buf[0] = (server->seq & (0xff << 8)) >> 8;
	buf[1] = server->seq & 0xff;

	if (server->client1.addr.sin_addr.S_un.S_addr != 0) {
		sendto(server->udp, buf, 18, 0, (struct sockaddr*)&server->client1.addr, sizeof(struct sockaddr));
	}

	if (server->client2.addr.sin_addr.S_un.S_addr != 0) {
		sendto(server->udp, buf, 18, 0, (struct sockaddr*)&server->client2.addr, sizeof(struct sockaddr));
	}
}

void server_destroy(struct Server* server) {
	WSACleanup();
	closesocket(server->pfds[0].fd);
	closesocket(server->pfds[1].fd);
	closesocket(server->pfds[2].fd);
	closesocket(server->udp);
	free(server);
}