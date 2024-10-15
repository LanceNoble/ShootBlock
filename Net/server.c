#include <stdio.h>

#include "comms.h"
#include "server.h"
#include "player.h"

struct Client {
	SOCKET waiter;
	struct Player* player;
};

static signed int status = 0;

static u_long blockMode = 1;
static SOCKET host = INVALID_SOCKET;
static struct addrinfo* addr = NULL;

static unsigned int numClients = 0;
static struct Client clients[MAX_PLAYERS];

static union Message* msg = NULL;

int server_create() {
	WSADATA wsaData;
	status = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (status != 0)
		return status;

	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	status = getaddrinfo(NULL, PORT, &hints, &addr);
	if (status != 0)
		return status;

	for (int i = 0; i < MAX_PLAYERS; i++) {
		clients[i].waiter = INVALID_SOCKET;
		clients[i].player = NULL;
	}

	msg = message_create();
	return status;
}

int server_start() {
	host = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (host == INVALID_SOCKET)
		return WSAGetLastError();
	if (ioctlsocket(host, FIONBIO, &blockMode) == SOCKET_ERROR)
		return WSAGetLastError();
	if (bind(host, addr->ai_addr, (int)addr->ai_addrlen) == SOCKET_ERROR)
		return WSAGetLastError();
	if (listen(host, SOMAXCONN) == SOCKET_ERROR)
		return WSAGetLastError();
	return 0;
}

void server_greet() {
	SOCKET waiter = accept(host, NULL, NULL);
	if (waiter == INVALID_SOCKET)
		return;
	if (numClients == MAX_PLAYERS) {
		closesocket(waiter);
		return;
	}
	if (ioctlsocket(waiter, FIONBIO, &blockMode) == SOCKET_ERROR) {
		closesocket(waiter);
		return;
	}

	struct Client* next = NULL;
	unsigned int nextId = 0;
	float nextXPos = 0.0f;
	float nextYPos = 0.0f;
	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (clients[i].player == NULL) {
			nextId = i + 1;
			// nextXPos = 
			// nextYPos = 
			clients[i].player = player_create(nextId, nextXPos, nextYPos);
			clients[i].waiter = waiter;
			next = &(clients[i]);
			break;
		}
	}

	if (next == NULL) {
		closesocket(waiter);
		return;
	}

	printf("Player %i Joined\n", nextId);
	message_format(msg, 1, nextId, 
		float_pack(nextXPos),
		float_pack(nextYPos));
	message_flip(msg);
	for (int i = 0; i < MAX_PLAYERS; i++)
		if (clients[i].player != NULL)
			printf("Telling Player %i that Player %i joined: %i\n", i + 1, nextId, message_send(msg, &(clients[i].waiter)));

	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (clients[i].player == NULL)
			continue;

		unsigned int oldId;
		float oldXPos;
		float oldYPos;
		player_get(clients[i].player, &oldId, &oldXPos, &oldYPos);

		if (oldId == nextId)
			continue;

		message_format(msg, 1, oldId, 
			float_pack(oldXPos), 
			float_pack(oldYPos));
		message_flip(msg);
		
		printf("Telling Player %i that Player %i exists: %i\n", nextId, i + 1, message_send(msg, &(next->waiter)));
	}

	printf("\n");

	numClients++;
}

void server_sync() {
	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (clients[i].player == NULL)
			continue;

		unsigned int typeActivity = 0;
		unsigned int idActivity = 0;
		unsigned int xPosActivity = 0;
		unsigned int yPosActivity = 0;
		int numBytes = message_fetch(msg, &(clients[i].waiter), &typeActivity, &idActivity, &xPosActivity, &yPosActivity);

		if (numBytes == 0 ||
			WSAGetLastError() == 10054) {

			printf("Player %i left\n", i + 1);

			closesocket(clients[i].waiter);
			player_destroy(&(clients[i].player));
			clients[i].waiter = INVALID_SOCKET;

			message_format(msg, 2, i + 1, 0, 0);
			message_flip(msg);
			for (int j = 0; j < MAX_PLAYERS; j++)
				if (clients[j].player != NULL)
					printf("Telling Player %i that Player %i left: %i\n", j + 1, i + 1, message_send(msg, &(clients[j].waiter)));

			printf("\n");

			numClients--;
		}
		else if (typeActivity == 3) {

			printf("Player %i moved\n", i + 1);

			player_place(clients[i].player, float_unpack(xPosActivity), float_unpack(yPosActivity));

			message_format(msg, typeActivity, idActivity, xPosActivity, yPosActivity);
			message_flip(msg);
			for (int j = 0; j < MAX_PLAYERS; j++) {
				if (clients[j].player != NULL) {
					printf("Telling Player %i that Player %i moved: %i\n", j + 1, i + 1, message_send(msg, &(clients[j].waiter)));

				}
					
			}
				
			printf("\n");
		}
	}
}

void server_stop() {
	status = 0;
	numClients = 0;
	for (int i = 0; i < MAX_PLAYERS; i++) {
		closesocket(clients[i].waiter);
		player_destroy(&(clients[i].player));
		clients[i].waiter = INVALID_SOCKET;
	}
	closesocket(host);
	host = INVALID_SOCKET;
	message_format(msg, 0, 0, 0, 0);
}

void server_destroy() {
	freeaddrinfo(addr);
	message_destroy(&msg);
	WSACleanup();
}
