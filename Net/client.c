#include "client.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include "comms.h"

struct Client { 
	SOCKET line;
	union Meta* info;
	union Bump* bump;
	unsigned char* data;
};

struct Client* client_create(const char *const ip, const char *const port, unsigned char** data, union Meta* info) {
	struct Client* client = malloc(sizeof(struct Client));
	if (client == NULL)
		return NULL;

	client->line = INVALID_SOCKET;
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
	struct addrinfo* serverAddress;
	if (getaddrinfo(ip, port, &hints, &serverAddress) != 0) {
		client_destroy(&client);
		return NULL;
	}

	client->line = socket(serverAddress->ai_family, serverAddress->ai_socktype, serverAddress->ai_protocol);
	if (connect(client->line, serverAddress->ai_addr, (long)serverAddress->ai_addrlen) == SOCKET_ERROR) {
		freeaddrinfo(serverAddress);
		client_destroy(&client);
		return NULL;
	}

	freeaddrinfo(serverAddress);

	client->info = malloc(sizeof(union Meta));
	if (client->info == NULL) {
		client_destroy(&client);
		return NULL;
	}

	short joinRes = recv(client->line, client->info->raw, sizeof(union Meta), 0);
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

	short installRes = recv(client->line, client->data, client->info->max * client->info->size, 0);
	flip(client->data, client->info->max * client->info->size);
	*data = client->data;
	*info = *(client->info);
	ioctlsocket(client->line, FIONBIO, &blockMode);
	return client;
}

void client_bump(struct Client* client, unsigned char id, unsigned char start, unsigned char size, unsigned char* val) {
	client->bump->id = id;
	client->bump->start = start;
	client->bump->size = size;
	for (int i = 0; i < client->bump->size; i++)
		client->bump->value[i] = val[i];

	flip(client->bump->raw, sizeof(union Bump));
	send(client->line, client->bump->raw, sizeof(union Bump), 0);
}

void client_sync(struct Client* client) {
	short numBytes = recv(client->line, client->bump->raw, sizeof(union Bump), 0);
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
	closesocket((*client)->line);
	free((*client)->data);
	(*client)->data = NULL;
	free((*client)->info);
	(*client)->info = NULL;
	free((*client)->bump);
	(*client)->bump = NULL;
	free(*client);
	*client = NULL;
}