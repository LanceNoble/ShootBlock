#include "comms.h"
#include "client.h"
#include "player.h"
#include <stdio.h>

static signed int status = 0;

static u_long blockMode = 1;
static SOCKET guest = INVALID_SOCKET;
static struct addrinfo* addr = NULL;

static struct Player* players[MAX_PLAYERS];
static struct Player* player = NULL;
static union Msg* msg = NULL;

int client_create() {
	WSADATA wsaData;
	status = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (status != 0)
		return status;

	for (int i = 0; i < MAX_PLAYERS; i++)
		players[i] = NULL;

	msg = msg_create();
	return status;
}
int client_join(char* ip) {
	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	status = getaddrinfo(ip, PORT, &hints, &addr);
	if (status != 0)
		return status;
	guest = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (guest == INVALID_SOCKET)
		return WSAGetLastError();
	if (connect(guest, addr->ai_addr, (int)addr->ai_addrlen) == SOCKET_ERROR)
		return WSAGetLastError();
	if (ioctlsocket(guest, FIONBIO, &blockMode) == SOCKET_ERROR)
		return WSAGetLastError();

	return 0;
}
int client_sync(struct Player** p, struct Player*** ps, unsigned int* numPlayers) {
	//if (player != NULL) {
	//	int id;
	//	player_get(player, &id, NULL, NULL);
	//	printf("You are Player %i\n\n", id);
	//}
	unsigned int type;
	unsigned int id;
	unsigned int xPos;
	unsigned int yPos;
	int numBytes = msg_fetch(msg, &guest, &type, &id, &xPos, &yPos);

	if (numBytes == SOCKET_ERROR || numBytes == 0)
		return numBytes;

	if (type == 1) {
		players[id - 1] = player_create(id, float_unpack(xPos), float_unpack(yPos));
		if (player == NULL)
			player = players[id - 1];
	}
	else if (type == 2)
		player_destroy(&(players[id - 1]));
	else if (type == 3)
		player_place(players[id - 1], float_unpack(xPos), float_unpack(yPos));

	if (p != NULL)
		*p = player;
	if (ps != NULL)
		*ps = players;
	if (numPlayers != NULL)
		*numPlayers = MAX_PLAYERS;

	return numBytes;
}
void client_notify(float x, float y) {
	if (player == NULL)
		return;
	unsigned int id;
	player_get(player, &id, NULL, NULL);
	msg_format(msg, 3, id, float_pack(x), float_pack(y));
	msg_flip(msg);
	msg_send(msg, &guest);
}
void client_leave() {
	status = 0;
	for (int i = 0; i < MAX_PLAYERS; i++)
		player_destroy(&(players[i]));
	freeaddrinfo(addr);
	closesocket(guest);
	guest = INVALID_SOCKET;
	player = NULL;
	msg_format(msg, 0, 0, 0, 0);
}
void client_destroy() {
	msg_destroy(&msg);
	WSACleanup();
}