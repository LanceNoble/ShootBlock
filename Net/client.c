#include "client.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include "comms.h"

struct Client { 
	SOCKET socket;
	unsigned char you[1];
	unsigned char data[SYNC_SIZE];
};

struct Client* client_create(const char *const ip, const char *const port, unsigned char** data) {
	struct Client* client = malloc(sizeof(struct Client));
	if (client == NULL)
		return NULL;

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

	client->socket = socket(serverAddress->ai_family, serverAddress->ai_socktype, serverAddress->ai_protocol);
	if (connect(client->socket, serverAddress->ai_addr, (long)serverAddress->ai_addrlen) == SOCKET_ERROR) {
		freeaddrinfo(serverAddress);
		client_destroy(&client);
		return NULL;
	}

	freeaddrinfo(serverAddress);

	// This is where the client either get's a message telling them which player in the server they are
	// Or they get kicked because the server's full
	unsigned short numBytes = recv(client->socket, client->you, 1, 0);
	unsigned long blockMode = 1;
	if (numBytes == 0 || numBytes == SOCKET_ERROR || ioctlsocket(client->socket, FIONBIO, &blockMode) == SOCKET_ERROR) {
		client_destroy(&client);
		return NULL;
	}

	for (int i = 0; i < SERVER_MAX; i++)
		client->data[i * PLAYER_SIZE] = CON_BYTE_OFF;

	*data = client->data;
	return client;
}

unsigned short client_sync(struct Client* client) {
	unsigned char buf[SYNC_SIZE];
	short numBytes = recv(client->socket, buf, SYNC_SIZE, 0);

	if (numBytes == 0 || WSAGetLastError() == 10053) {
		client_destroy(&client);
		return 420;
	}

	if (numBytes == SOCKET_ERROR) {
		return 420;
	}

	if (numBytes == SYNC_SIZE) {
		flip_bytes(buf, SYNC_SIZE);
		for (int i = 0; i < SYNC_SIZE; i++) {
			client->data[i] = buf[i];
		}
		return numBytes;
	}
	
	flip_bytes(buf, SYNC_SIZE);
	union Bump bump;
	for (int i = 59, j = 0; i < 64; i++, j++) {
		bump.raw[j] = buf[i];
	}

	for (int i = bump.start, j = 0; i < bump.start + (bump.size); i++, j++)
		client->data[bump.id * PLAYER_SIZE + i] = bump.value[j];

	return numBytes;
}

unsigned short client_bump(struct Client* client, unsigned char start, unsigned char size, unsigned char* val) {
	if (start == CON_BYTE)
		return -420;

	union Bump bump;
	bump.id = client->you[0];
	bump.start = start;
	bump.size = size;
	for (int i = 0; i < (bump.size); i++)
		bump.value[i] = val[i];

	flip_bytes(bump.raw, sizeof(bump));

	return send(client->socket, bump.raw, sizeof(bump), 0);
}

void client_destroy(struct Client** client) {
	closesocket((*client)->socket);
	free(*client);
	*client = NULL;
}