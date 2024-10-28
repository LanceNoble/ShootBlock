#include "client.h"

#include <winsock2.h>
#include <time.h>
#include <stdio.h>

#include "comms.h"

struct Host {
	struct sockaddr_in addr;
	unsigned long elapse;
};

struct Client {
	unsigned long id;
	SOCKET udp;
	struct Host server;
};

struct Client* client_create(unsigned long byte0, unsigned long byte1, unsigned long byte2, unsigned long byte3, unsigned short port) {
	struct Client* client = malloc(sizeof(struct Client));
	if (client == NULL) {
		return NULL;
	}
	client->id = PID;
	client->udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	struct sockaddr_in addrBind;
	addrBind.sin_family = AF_INET;
	addrBind.sin_addr.S_un.S_addr = INADDR_ANY;
	addrBind.sin_port = 0;

	if (bind(client->udp, (struct sockaddr*)&addrBind, (unsigned long)sizeof(struct sockaddr)) == SOCKET_ERROR) {
		printf("failed to bind socket\n");
		client_destroy(&client);
		return NULL;
	}

	unsigned long blockMode = 1;
	ioctlsocket(client->udp, FIONBIO, &blockMode);

	unsigned long address = (byte0 << 24) | (byte1 << 16) | (byte2 << 8) | byte3;

	client->server.addr.sin_family = AF_INET;
	client->server.addr.sin_addr.S_un.S_addr = htonl(address);
	client->server.addr.sin_port = htons(port);

	client->server.elapse = clock();
	return client;
}

void client_ping(struct Client* client) {
	if (client == NULL) {
		return;
	}
	union Data data;
	data.pid = client->id;
	flip(data.raw, sizeof(union Data));
	struct sockaddr* a = (struct sockaddr*)(&(client->server.addr));
	sendto(client->udp, data.raw, sizeof(union Data), 0, a, (unsigned long)sizeof(struct sockaddr));
}

void client_sync(struct Client* client) {
	if (client == NULL) {
		return;
	}
	if ((clock() - client->server.elapse) / (double)CLOCKS_PER_SEC >= TIMEOUT) {
		printf("Server died\n");
		client_destroy(&client);
		return;
	}

	union Data from;
	struct sockaddr_in fromAddr;
	unsigned long fromAddrLen = sizeof(struct sockaddr);
	short numBytes = recvfrom(client->udp, from.raw, (unsigned long)sizeof(union Data), 0, (struct sockaddr*)&fromAddr, &fromAddrLen);
	if (numBytes == SOCKET_ERROR || fromAddr.sin_addr.S_un.S_addr != client->server.addr.sin_addr.S_un.S_addr) {
		return;
	}

	flip(from.raw, sizeof(union Data));
	if (from.pid > client->id) {
		client->id = from.pid;
	}
	if (from.pid != client->id) {
		return;
	}

	printf("%lu.%lu.%lu.%lu:%lu Says: %i\n", 
		fromAddr.sin_addr.S_un.S_un_b.s_b1,
		fromAddr.sin_addr.S_un.S_un_b.s_b2,
		fromAddr.sin_addr.S_un.S_un_b.s_b3,
		fromAddr.sin_addr.S_un.S_un_b.s_b4,
		ntohs(fromAddr.sin_port),
		from.pid);

	client->server.elapse = clock();
}

void client_destroy(struct Client** client) {
	closesocket((*client)->udp);
	free(*client);
	*client = NULL;
}