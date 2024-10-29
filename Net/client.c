#include "client.h"

#include <winsock2.h>
#include <time.h>
#include <stdio.h>

#include "comms.h"

struct Client {
	unsigned long long udp;
	struct sockaddr_in addr;
	unsigned long elapse;
	unsigned long seq;
};

short client_create(unsigned char b0, unsigned char b1, unsigned char b2, unsigned char b3, unsigned short port, struct Client** client) {
	*client = malloc(sizeof(struct Client));
	if (*client == NULL) {
		return -1;
	}

	(*client)->udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if ((*client)->udp == INVALID_SOCKET) {
		return WSAGetLastError();
	}

	unsigned long mode = 1;
	if (ioctlsocket((*client)->udp, FIONBIO, &mode) == SOCKET_ERROR) {
		return WSAGetLastError();
	}

	struct sockaddr_in binder;
	binder.sin_family = AF_INET;
	binder.sin_addr.S_un.S_addr = INADDR_ANY;
	binder.sin_port = 0;

	if (bind((*client)->udp, (struct sockaddr*)&binder, (unsigned long)sizeof(struct sockaddr)) == SOCKET_ERROR) {
		return WSAGetLastError();
	}

	(*client)->addr.sin_family = AF_INET;
	(*client)->addr.sin_addr.S_un.S_addr = htonl((b0 << 24) | (b1 << 16) | (b2 << 8) | b3);
	(*client)->addr.sin_port = htons(port);

	(*client)->elapse = clock();
	(*client)->seq = 0;
	return 0;
}

short client_ping(struct Client* client) {
	union Data to;
	to.pid = PID;
	to.seq = client->seq;
	flip(to.raw, sizeof(union Data));
	short numSend = sendto(client->udp, to.raw, sizeof(union Data), 0, (struct sockaddr*)&(client->addr), (unsigned long)sizeof(struct sockaddr));
	if (numSend == SOCKET_ERROR) {
		return WSAGetLastError();
	}
	printf("Sending Packet %lu\n", client->seq++);
	return numSend;
}

short client_sync(struct Client* client) {
	if ((clock() - client->elapse) / (double)CLOCKS_PER_SEC >= TIMEOUT) {
		return -1;
	}

	union Data from;
	struct sockaddr_in fromAddr;
	unsigned long fromAddrLen = sizeof(struct sockaddr);
	short numRecv = recvfrom(client->udp, from.raw, (unsigned long)sizeof(union Data), 0, (struct sockaddr*)&fromAddr, &fromAddrLen);
	if (numRecv == SOCKET_ERROR) {
		return WSAGetLastError();
	}
	if (fromAddr.sin_addr.S_un.S_addr != client->addr.sin_addr.S_un.S_addr) {
		return -2;
	}

	flip(from.raw, sizeof(union Data));
	if (from.pid != PID) {
		return -3;
	}

	client->elapse = clock();
	return numRecv;
}

void client_destroy(struct Client** client) {
	closesocket((*client)->udp);
	free(*client);
	*client = NULL;
}