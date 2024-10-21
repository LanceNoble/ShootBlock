#include "client.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include "comms.h"

struct Client { 
	SOCKET tcp;
	SOCKET udp;
	union Meta* info;
	union Bump* bump;
	unsigned char* data;
};

struct Client* client_create(const char *const ip, const char *const port, unsigned char** data, union Meta* info) {
	struct Client* client = malloc(sizeof(struct Client));
	if (client == NULL)
		return NULL;

	client->tcp = INVALID_SOCKET;


	client->info = NULL;
	client->bump = NULL;
	client->data = NULL;
	client->bump = malloc(sizeof(union Bump));
	if (client->bump == NULL) {
		client_destroy(&client);
		return NULL;
	}

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

	client->tcp = socket(tcpAddr->ai_family, tcpAddr->ai_socktype, tcpAddr->ai_protocol);
	if (connect(client->tcp, tcpAddr->ai_addr, (long)tcpAddr->ai_addrlen) == SOCKET_ERROR) {
		freeaddrinfo(tcpAddr);
		client_destroy(&client);
		return NULL;
	}

	freeaddrinfo(tcpAddr);

	client->info = malloc(sizeof(union Meta));
	if (client->info == NULL) {
		client_destroy(&client);
		return NULL;
	}

	short joinRes = recv(client->tcp, client->info->raw, sizeof(union Meta), 0);
	unsigned long blockMode = 1;
	if (joinRes == 0 || joinRes == SOCKET_ERROR) {
		client_destroy(&client);
		return NULL;
	}

	client->data = malloc(sizeof(unsigned char) * client->info->max * client->info->size);
	if (client->data == NULL) {
		client_destroy(&client);
		return NULL;
	}

	short installRes = recv(client->tcp, client->data, client->info->max * client->info->size, 0);
	flip(client->data, client->info->max * client->info->size);
	*data = client->data;
	*info = *(client->info);
	ioctlsocket(client->tcp, FIONBIO, &blockMode);
	return client;
}

void client_bump(struct Client* client, unsigned char id, unsigned char start, unsigned char size, unsigned char* val) {
	client->bump->id = id;
	client->bump->start = start;
	client->bump->size = size;
	for (int i = 0; i < client->bump->size; i++)
		client->bump->value[i] = val[i];

	flip(client->bump->raw, sizeof(union Bump));
	send(client->tcp, client->bump->raw, sizeof(union Bump), 0);
}

void client_sync(struct Client* client) {
	short numBytes = recv(client->tcp, client->bump->raw, sizeof(union Bump), 0);
	if (numBytes == 0 || WSAGetLastError() == 10053) {
		client_destroy(&client);
		return;
	}
	if (numBytes == SOCKET_ERROR) {
		return;
	}

	flip(client->bump->raw, sizeof(union Bump));
	for (int i = client->bump->start, j = 0; i < client->bump->start + client->bump->size; i++, j++) {
		client->data[client->bump->id * client->info->size + i] = client->bump->value[j];
	}
}

void client_destroy(struct Client** client) {
	closesocket((*client)->tcp);
	free((*client)->data);
	(*client)->data = NULL;
	free((*client)->info);
	(*client)->info = NULL;
	free((*client)->bump);
	(*client)->bump = NULL;
	free(*client);
	*client = NULL;
}