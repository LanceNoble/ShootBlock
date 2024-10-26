#include "client.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <time.h>

#include "comms.h"

struct Client { 
	SOCKET tcp;
	SOCKET udp;

	union Meta info;
	struct addrinfo* udpAddr;

	clock_t start;
	unsigned char szData;
	unsigned char* cliData;
	unsigned char* serData;

	unsigned long seq;
	struct Action* firstAct;
	struct Action* lastAct;
};

struct Client* client_create(const char *const ip, const char *const port, unsigned char** data, union Meta* info) {
	struct Client* client = malloc(sizeof(struct Client));
	if (client == NULL) {
		return NULL;
	}

	client->firstAct = malloc(sizeof(struct Action));
	if (client->firstAct == NULL) {
		client_destroy(&client);
		return NULL;
	}
	client->lastAct = client->firstAct;

	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;

	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	struct addrinfo* tcpAddr;
	if (getaddrinfo(ip, port, &hints, &tcpAddr) != 0) {
		client_destroy(&client);
		return NULL;
	}

	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	if (getaddrinfo(ip, port, &hints, &(client->udpAddr)) != 0) {
		client_destroy(&client);
		return NULL;
	}

	client->tcp = socket(tcpAddr->ai_family, tcpAddr->ai_socktype, tcpAddr->ai_protocol);
	if (client->tcp == INVALID_SOCKET) {
		freeaddrinfo(tcpAddr);
		client_destroy(&client);
		return NULL;
	}

	client->udp = socket(client->udpAddr->ai_family, client->udpAddr->ai_socktype, client->udpAddr->ai_protocol);
	if (client->udp == INVALID_SOCKET) {
		client_destroy(&client);
		return NULL;
	}

	if (connect(client->tcp, tcpAddr->ai_addr, (long)tcpAddr->ai_addrlen) == SOCKET_ERROR) {
		freeaddrinfo(tcpAddr);
		client_destroy(&client);
		return NULL;
	}
	freeaddrinfo(tcpAddr);

	short join = recv(client->tcp, client->info.raw, sizeof(union Meta), 0);
	if (join == 0 || join == SOCKET_ERROR) {
		client_destroy(&client);
		return NULL;
	}
	client->seq = 0;
	client->firstAct->bump.seq = client->seq;
	client->firstAct->bump.id = client->info.you;
	client->firstAct->bump.st = CON_BYTE;
	client->firstAct->bump.sz = CON_BYTE_SIZE;
	client->firstAct->bump.val[CON_BYTE] = CON_BYTE_ON;
	client->firstAct->next = NULL;

	// +4 to include the version of the player data the server sent
	client->szData = 4 + (client->info.max * client->info.sz);
	client->cliData = malloc(sizeof(unsigned char) * client->szData);
	client->serData = malloc(sizeof(unsigned char) * client->szData);
	if (client->cliData == NULL || client->serData == NULL) {
		client_destroy(&client);
		return NULL;
	}
	client->cliData[0] = 0;
	client->cliData[1] = 0;
	client->cliData[2] = 0;
	client->cliData[3] = 0;

	unsigned long blockMode = 1;
	ioctlsocket(client->tcp, FIONBIO, &blockMode);
	ioctlsocket(client->udp, FIONBIO, &blockMode);

	*data = client->cliData;
	*info = client->info;

	client->start = clock();
	return client;
}

void client_bump(struct Client* client) {
	union Bump bump;
	bump = client->firstAct->bump;
	flip(bump.raw, sizeof(union Bump));
	sendto(client->udp, bump.raw, sizeof(union Bump), 0, client->udpAddr, (unsigned long)(client->udpAddr->ai_addrlen));
}

void client_queue(struct Client* client, unsigned char id, unsigned char start, unsigned char size, unsigned char* val) {
	struct Action* newAct = malloc(sizeof(struct Action));
	if (newAct == NULL) {
		return;
	}
	newAct->bump.seq = client->seq + 1;
	newAct->bump.id = id;
	newAct->bump.st = start;
	newAct->bump.sz = size;
	newAct->next = NULL;
	for (unsigned char i = 0; i < newAct->bump.sz; i++) {
		newAct->bump.val[i] = val[i];
	}
	if (client->lastAct == NULL) {
		client->firstAct = newAct;
		client->lastAct = newAct;
	}
	else if (client->lastAct != NULL) {
		client->lastAct->next = newAct;
		client->lastAct = client->lastAct->next;
	}
}

void client_sync(struct Client* client) {
	clock_t elapsed = clock() - client->start;
	if (elapsed / CLOCKS_PER_SEC >= 10) {
		client_destroy(&client);
		return;
	}

	short data = recvfrom(client->udp, client->serData, client->szData, 0, NULL, NULL);
	if (data == SOCKET_ERROR) {
		return;
	}

	client->start = clock();
	flip(client->serData, client->szData);
	unsigned long serVer = (client->serData[0] << 24) | (client->serData[1] << 16) | (client->serData[2] << 8) | client->serData[3];
	unsigned long cliVer = (client->cliData[0] << 24) | (client->cliData[1] << 16) | (client->cliData[2] << 8) | client->cliData[3];
	if (serVer >= cliVer) {
		for (unsigned char i = 0; i < client->szData; i++) {
			client->cliData[i] = client->serData[i];
		}
		if (serVer > cliVer) {
			struct Action* tempHead = client->firstAct;
			client->firstAct = client->firstAct->next;
			free(client->firstAct);
			client->firstAct = NULL;
		}
	}
}

static void action_free(struct Action* act) {
	if (act == NULL) {
		return;
	}
	action_free(act->next);
	free(act);
	act = NULL;
}

void client_destroy(struct Client** client) {
	closesocket((*client)->tcp);
	closesocket((*client)->udp);

	freeaddrinfo((*client)->udpAddr);
	(*client)->udpAddr = NULL;

	free((*client)->cliData);
	(*client)->cliData = NULL;

	free((*client)->serData);
	(*client)->serData = NULL;

	action_free((*client)->firstAct);

	free(*client);
	*client = NULL;
}