#include "server.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include "comms.h"

struct Player {
	struct sockaddr udpAddr;

	unsigned long seq;
	struct Action* firstAct;
	struct Action* lastAct;
};

struct Spot {
	unsigned char val;
	struct Spot* next;
};

struct Server {
	unsigned char max;
	unsigned char size;
	unsigned char numPlayers;

	unsigned char* data;
	struct Player** players;
	struct pollfd* monitors;
	//union Bump* bumps;

	struct Spot* firstOpen;
	struct Spot* lastOpen;
};

static unsigned long noBlockMode = 1;

struct Server* server_create(unsigned char max, unsigned char size, const char* const port) {
	struct Server* server = malloc(sizeof(struct Server));
	if (server == NULL)
		return NULL;

	server->max = max;
	server->size = size;
	server->numPlayers = 0;

	server->data = malloc(sizeof(unsigned char) * (4 + (server->max * server->size)));
	if (server->data == NULL) {
		server_destroy(&server);
		return NULL;
	}
	for (int i = 0; i < server->max; i++) {
		server->data[4 + (i * server->size)] = CON_BYTE_OFF;
	}

	server->players = malloc(sizeof(struct Player*) * server->max);
	for (unsigned char i; i < server->max; i++) {
		server->players[i] = NULL;
	}

	server->monitors = malloc(sizeof(struct pollfd) * (2 + server->max));
	if (server->monitors == NULL) {
		server_destroy(&server);
		return NULL;
	}

	/*
	server->bumps = malloc(sizeof(union Bump) * server->max);
	if (server->bumps == NULL) {
		server_destroy(&server);
		return NULL;
	}
	*/
	
	server->firstOpen = malloc(sizeof(struct Spot));
	if (server->firstOpen == NULL) {
		server_destroy(&server);
		return NULL;
	}
	server->firstOpen->val = 0;
	server->lastOpen = server->firstOpen;
	for (int i = 1; i < server->max; i++) {
		server->lastOpen->next = malloc(sizeof(struct Spot));
		if (server->lastOpen->next == NULL) {
			server_destroy(&server);
			return NULL;
		}
		server->lastOpen->next->val = i;
		server->lastOpen = server->lastOpen->next;
	}
	server->lastOpen->next = NULL;
	
	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	struct addrinfo* bindAddress;
	if (getaddrinfo(NULL, port, &hints, &bindAddress) != 0) {
		server_destroy(&server);
		return NULL;
	}

	for (int i = 0; i < 1 + server->max; i++) {
		if (i == 0) {
			server->monitors[0].fd = socket(bindAddress->ai_family, bindAddress->ai_socktype, bindAddress->ai_protocol);
			server->monitors[0].revents = 0;
		}
		server->monitors[i].events = POLLIN;
	}

	if (ioctlsocket(server->monitors[0].fd, FIONBIO, &noBlockMode) == SOCKET_ERROR) {
		freeaddrinfo(bindAddress);
		server_destroy(&server);
		return NULL;
	}

	if (bind(server->monitors[0].fd, bindAddress->ai_addr, (long)bindAddress->ai_addrlen) == SOCKET_ERROR) {
		freeaddrinfo(bindAddress);
		server_destroy(&server);
		return NULL;
	}

	freeaddrinfo(bindAddress);

	if (listen(server->monitors[0].fd, SOMAXCONN) == SOCKET_ERROR) {
		server_destroy(&server);
		return NULL;
	}	

	return server;
}

void server_relay(struct Server* server) {
	// unsigned char numBumps = 0;
	WSAPoll(server->monitors, 1 + server->max, -1);
	for (unsigned char i = 0; i < 1 + server->max; i++) {
		if (!(server->monitors[i].revents & POLLIN) && !(server->monitors[i].revents & POLLHUP)) {
			continue;
		}

		// -1 to exclude the server socket
		if (i == 0 && server->numPlayers == server->max) {
			closesocket(accept(server->monitors[0].fd, NULL, NULL));
		}
		else if (i == 0 && server->numPlayers < server->max) {
			unsigned char nextVacancy = server->firstOpen->val;
			struct Spot* recentOccupation = server->firstOpen;
			server->firstOpen = server->firstOpen->next;
			free(recentOccupation);

			server->monitors[nextVacancy + 1].fd = accept(server->monitors[0].fd, NULL, NULL);
			ioctlsocket(server->monitors[nextVacancy + 1].fd, FIONBIO, &noBlockMode);

			server->bumps[numBumps].id = nextVacancy;
			server->bumps[numBumps].st = CON_BYTE;
			server->bumps[numBumps].sz = CON_BYTE_SIZE;
			server->bumps[numBumps].val[CON_BYTE_SIZE - 1] = CON_BYTE_ON;
			numBumps++;

			union Meta meta;
			meta.max = server->max;
			meta.sz = server->size;
			meta.you = nextVacancy;
			send(server->monitors[nextVacancy + 1].fd, meta.raw, sizeof(union Meta), 0);
			
			server->numPlayers++;
		}
		else if (server->monitors[i].revents & POLLHUP) {
			server->lastOpen->next = malloc(sizeof(struct Spot));
			if (server->lastOpen->next == NULL) {
				server_destroy(&server);
				return;
			}
			server->lastOpen->next->val = i - 1;
			server->lastOpen = server->lastOpen->next;

			server->bumps[numBumps].id = i - 1;
			server->bumps[numBumps].st = CON_BYTE;
			server->bumps[numBumps].sz = CON_BYTE_SIZE;
			server->bumps[numBumps].val[CON_BYTE_SIZE - 1] = CON_BYTE_OFF;
			numBumps++;

			closesocket(server->monitors[i].fd);
			server->monitors[i].fd = INVALID_SOCKET;

			server->numPlayers--;
		}
		else if (server->monitors[i].revents & POLLIN) {
			unsigned short numBytes = recv(server->monitors[i].fd, server->bumps[numBumps].raw, sizeof(union Bump), 0);
			flip(server->bumps[numBumps].raw, sizeof(union Bump));
			numBumps++;
		}
	}

	for (unsigned char i = 0; i < numBumps; i++) {
		for (unsigned char j = server->bumps[i].st, k = 0; j < server->bumps[i].st + server->bumps[i].sz; j++, k++) {
			server->data[server->bumps[i].id * server->size + j] = server->bumps[i].val[k];
		}
		union Bump bumpCopy;
		bumpCopy.id = server->bumps[i].id;
		bumpCopy.st = server->bumps[i].st;
		bumpCopy.sz = server->bumps[i].sz;
		for (unsigned char j = 0; j < MAX_UPDATE_SIZE; j++) {
			bumpCopy.val[j] = server->bumps[i].val[j];
		}
		flip(bumpCopy.raw, sizeof(union Bump));
		for (unsigned char j = 0; j < server->max; j++) {
			if (server->data[j * server->size] == CON_BYTE_ON && j != server->bumps[i].id) {
				// +1 to exclude the server socket
				send(server->monitors[j + 1].fd, bumpCopy.raw, sizeof(union Bump), 0);
			}
		}
		if (server->bumps[i].st != CON_BYTE) {
			continue;
		}
		unsigned char* installation = malloc(sizeof(unsigned char) * server->max * server->size);
		if (installation == NULL) {
			server_destroy(&server);
			return;
		}
		for (unsigned char j = 0; j < server->max * server->size; j++) {
			installation[j] = server->data[j];
		}
		flip(installation, server->max * server->size);
		send(server->monitors[server->bumps[i].id + 1].fd, installation, server->max * server->size, 0);
		free(installation);
	}
}

static void free_linked(struct Spot* s) {
	if (s == NULL)
		return;
	free_linked(s->next);
	free(s);
}

void server_destroy(struct Server** server) {
	for (unsigned char i = 0; i < 1 + SERVER_MAX; i++)
		closesocket((*server)->monitors->fd);
	free((*server)->data);
	(*server)->data = NULL;
	free((*server)->monitors);
	(*server)->monitors = NULL;
	free((*server)->bumps);
	(*server)->bumps = NULL;
	free_linked((*server)->firstOpen);
	free(*server);
	*server = NULL;
}
