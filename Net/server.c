#include "server.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include "comms.h"

struct Spot {
	unsigned char val;
	struct Spot* next;
};

struct Server {
	unsigned char max;
	unsigned char numPlayers;
	unsigned char data[SYNC_SIZE];
	union Bump bumps[SERVER_MAX];

	// +1 to include the server's socket
	struct pollfd monitors[1 + SERVER_MAX];

	struct Spot* longestVacancy;
	struct Spot* shortestVacancy;
};

static unsigned long noBlockMode = 1;

struct Server* server_create(const char* const port, unsigned char max, unsigned char** data, union Bump** bumps) {

	struct Server* server = malloc(sizeof(struct Server));
	if (server == NULL)
		return NULL;

	server->max = max;
	server->numPlayers = 0;
	for (int i = 0; i < SERVER_MAX; i++)
		server->data[i * PLAYER_SIZE] = CON_BYTE_OFF;
	*data = server->data;
	*bumps = server->bumps;

	server->longestVacancy = malloc(sizeof(struct Spot));
	if (server->longestVacancy == NULL) {
		server_destroy(&server);
		return NULL;
	}
	server->longestVacancy->val = 0;
	server->shortestVacancy = server->longestVacancy;
	for (int i = 1; i < SERVER_MAX; i++) {
		server->shortestVacancy->next = malloc(sizeof(struct Spot));
		if (server->shortestVacancy->next == NULL) {
			server_destroy(&server);
			return NULL;
		}
		server->shortestVacancy->next->val = i;
		server->shortestVacancy = server->shortestVacancy->next;
	}

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

	for (int i = 0; i < 1 + SERVER_MAX; i++) {
		if (i == 0) {
			server->monitors[0].fd = socket(bindAddress->ai_family, bindAddress->ai_socktype, bindAddress->ai_protocol);
			server->monitors[0].events = POLLIN;
			server->monitors[0].revents = 0;
			continue;
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

void server_sync(struct Server* server, unsigned char* numBumpsPtr) {
	unsigned char numBumps = 0;
	WSAPoll(server->monitors, 1 + SERVER_MAX, -1);
	for (unsigned char i = 0; i < 1 + SERVER_MAX; i++) {
		if (!(server->monitors[i].revents & POLLIN) && !(server->monitors[i].revents & POLLHUP))
			continue; 

		// -1 to exclude the server socket
		if (i == 0 && server->numPlayers == server->max)
			closesocket(accept(server->monitors[0].fd, NULL, NULL));
		else if (i == 0 && server->numPlayers < server->max) {
			unsigned char nextVacancy = server->longestVacancy->val;
			struct Spot* recentOccupation = server->longestVacancy;
			server->longestVacancy = server->longestVacancy->next;
			free(recentOccupation);

			server->monitors[nextVacancy + 1].fd = accept(server->monitors[0].fd, NULL, NULL);
			server->monitors[nextVacancy + 1].events = POLLIN;// | POLLHUP;
			ioctlsocket(server->monitors[nextVacancy + 1].fd, FIONBIO, &noBlockMode);

			server->bumps[numBumps].id = nextVacancy;
			server->bumps[numBumps].start = CON_BYTE;
			server->bumps[numBumps].size = CON_BYTE_SIZE;
			server->bumps[numBumps].value[CON_BYTE_SIZE - 1] = CON_BYTE_ON;
			numBumps++;

			char buf[1];
			buf[0] = nextVacancy;
			send(server->monitors[nextVacancy + 1].fd, buf, 1, 0);

			buf[0] = CON_BYTE_ON;
			server_bump(server, nextVacancy, CON_BYTE, CON_BYTE_SIZE, buf);
			
			server->numPlayers++;
		}
		else if (server->monitors[i].revents & POLLHUP) {
			server->shortestVacancy->next = malloc(sizeof(struct Spot));
			if (server->shortestVacancy->next == NULL) {
				server_destroy(&server);
				return;
			}
			server->shortestVacancy->next->val = i - 1;
			server->shortestVacancy = server->shortestVacancy->next;

			server->bumps[numBumps].id = i - 1;
			server->bumps[numBumps].start = CON_BYTE;
			server->bumps[numBumps].size = CON_BYTE_SIZE;
			server->bumps[numBumps].value[CON_BYTE_SIZE - 1] = CON_BYTE_OFF;
			numBumps++;

			closesocket(server->monitors[i].fd);

			char buf[1];
			buf[0] = CON_BYTE_OFF;
			server_bump(server, i - 1, CON_BYTE, CON_BYTE_SIZE, buf);
		}
		else if (server->monitors[i].revents & POLLIN) {
			unsigned short numBytes = recv(server->monitors[i].fd, server->bumps[numBumps].raw, sizeof(union Bump), 0);
			flip_bytes(server->bumps[numBumps].raw, sizeof(union Bump));
			numBumps++;
		}
	}
	*numBumpsPtr = numBumps;
}

void server_bump(struct Server* server, unsigned char id, unsigned char start, unsigned char size, unsigned char* val) {
	for (int i = start, j = 0; i < start + size; i++, j++)
		server->data[id * PLAYER_SIZE + i] = val[j];

	union Bump bump;
	bump.id = id;
	bump.start = start;
	bump.size = size;
	for (int i = 0; i < bump.size; i++)
		bump.value[i] = val[i];

	flip_bytes(bump.raw, sizeof(bump));

	for (int i = 0; i < SERVER_MAX; i++) {
		if (server->data[i * PLAYER_SIZE] == CON_BYTE_ON && i != id) {
			// +1 to exclude the server socket
			send(server->monitors[i + 1].fd, bump.raw, sizeof(bump), 0); 
		}
	}
	
	if (start != CON_BYTE)
		return;

	unsigned char installation[SYNC_SIZE];
	for (int i = 0; i < SYNC_SIZE; i++) {
		installation[i] = server->data[i];
	}
	flip_bytes(installation, SYNC_SIZE);
	send(server->monitors[id + 1].fd, installation, SYNC_SIZE, 0);
}

void server_destroy(struct Server** server) {
	for (unsigned char i = 0; i < 1 + SERVER_MAX; i++)
		closesocket((*server)->monitors->fd);
	free(*server);
	*server = NULL;
}
