// 10/08/2024
// This program is the server portion of my attempt at writing netcode
// 
// Successful Data Syncs:
// - List of connected players
// - Player positions
// - 
// 
// TODO:
// 1. 

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib") 

#define MAX_PLAYERS 3
#define VACANT_PLAYER 0

struct player {
	SOCKET waiter;
	unsigned int id;
	signed int xPos;
	signed int yPos;
};

// How the client and server will MOSTLY format their messages when talking to each other
// The netcode is written in a way where the client and server won't always use this format:
// 1. a client's connect() also means Player Join
// 2. a client's or server's closesocket() also means Player Leave
// 
// The union lets us represent the message in 3 forms:
// 1. Struct Form makes it easier to format the message
// 2. Int Form makes it easier to use hton and ntoh functions
// 3. Char Array Form makes it easier to send and recv
//
// Not all message types will utilize all bit fields
union msg {
	struct {
		// 1 - Player Join
		// 2 - Player Leave
		// 3 - Player Move
		unsigned int type : 2;

		// ID of player who sent the message
		unsigned int id : 2;

		// Position of player who sent the message
		signed int xPos : 14;
		signed int yPos : 14;
	};
	int whole;
	char raw[4];
};

// Create a message ready for send or recv
union msg formatmsg(unsigned int type, unsigned int id, signed int xPos, signed int yPos) {
	union msg msg;
	msg.type = type;
	msg.id = id;
	msg.xPos = xPos;
	msg.yPos = yPos;
	return msg;
}

// Create a socket ready to listen for and accept connections
// addr is a pointer to a pointer because getaddrinfo changes what addr points to
// Everything is pass-by-value in C, so passing in just a pointer creates a whole new pointer
// So getaddrinfo would actually operate on that whole new pointer, not your pointer
// The pointer to a pointer ensures that your pointer can still be referenced
int inithost(int type, int proto, struct addrinfo** addr, SOCKET* host) {
	int status;
	u_long mode = 1;
	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = type;
	hints.ai_protocol = proto;
	hints.ai_flags = AI_PASSIVE;
	status = getaddrinfo(NULL, "3490", &hints, addr);
	if (status != 0)
		return status;
	*host = socket((*addr)->ai_family, (*addr)->ai_socktype, (*addr)->ai_protocol);
	if (*host == INVALID_SOCKET) {
		status = WSAGetLastError();
		return status;
	}
	if (ioctlsocket(*host, FIONBIO, &mode) == SOCKET_ERROR) {
		status = WSAGetLastError();
		return status;
	}
	if (bind(*host, (*addr)->ai_addr, (*addr)->ai_addrlen) == SOCKET_ERROR) {
		status = WSAGetLastError();
		return status;
	}
	return 0;
}

int main() {
	// Initialize player list
	// Zeroing the array is important because a zero'd player is used to represent an empty spot in the server
	struct player players[MAX_PLAYERS];
	ZeroMemory(players, sizeof(players));

	// Initialize WSA
	WSADATA wsaData;
	int status = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (status != 0)
		return status;

	// Initialize host sockets
	struct addrinfo* addr;
	SOCKET host;
	status = inithost(SOCK_STREAM, IPPROTO_TCP, &addr, &host);
	if (status != 0)
		goto cleanup;

	if (listen(host, SOMAXCONN) == SOCKET_ERROR) {
		status = WSAGetLastError();
		goto cleanup;
	}

	// The Great Server Loop
	int numPlayers = 0;
	while (!(GetAsyncKeyState(VK_END) & 0x01)) {

		// If the server's full, don't even bother accepting new sockets
		if (numPlayers == MAX_PLAYERS)
			goto sync;

		// If there's no incoming connection requests, skip straight to syncing
		SOCKET waiter = accept(host, NULL, NULL);
		if (waiter == INVALID_SOCKET)
			goto sync;

		// If the incoming player's socket cannot be non-blocking, kick it out
		u_long mode = 1;
		if (ioctlsocket(waiter, FIONBIO, &mode) == SOCKET_ERROR) {
			closesocket(waiter);
			goto sync;
		}

		// Look for the first open spot in the server for the new player
		struct player* next = NULL;
		for (int i = 0; i < MAX_PLAYERS; i++) {
			if (players[i].id == VACANT_PLAYER) {
				next = &(players[i]);
				next->waiter = waiter;
				next->id = i + 1;
				next->xPos = 0;
				next->yPos = 0;
				break;
			}
		}
		
		// Final check to see if the server is truly not full
		if (next == NULL) {
			closesocket(waiter);
			goto sync;
		}

		// The player has now officially joined the server
		// Signal other players that this player joined 
		// Also signal the one who just joined so they know which player they are
		union msg joiner = formatmsg(1, next->id, next->xPos, next->yPos);
		joiner.whole = htonl(joiner.whole);
		for (int i = 0; i < MAX_PLAYERS; i++)
			if (players[i].id != VACANT_PLAYER)
				send(players[i].waiter, joiner.raw, sizeof(joiner), 0);
		
		// Signal only the player who just joined about who already joined the server
		for (int i = 0; i < MAX_PLAYERS; i++) {
			if (players[i].id == next->id || 
				players[i].id == VACANT_PLAYER)
				continue;
			union msg existing = formatmsg(1, players[i].id, players[i].xPos, players[i].yPos);
			existing.whole = htonl(existing.whole);
			send(next->waiter, existing.raw, sizeof(existing), 0);
		}

		numPlayers++;

		// We are now done processing connections (if there were any)
		// Now we process in-game activity
	sync:
		for (int i = 0; i < MAX_PLAYERS; i++) {
			if (players[i].id == VACANT_PLAYER)
				continue;

			// Attempt to receive an activity signal from the player
			// Important to zero the union's recv buffer
			// Zero buffer is used to represent no activity
			union msg activity;
			activity.whole = 0;
			int numBytes = recv(players[i].waiter, activity.raw, sizeof(activity), 0);
			activity.whole = ntohl(activity.whole);

			// Check if the player disconnected cleanly or force closed the program
			if (numBytes == 0 ||
				WSAGetLastError() == 10054) {

				union msg leaver = formatmsg(2, players[i].id, 0, 0);
				closesocket(players[i].waiter);
				ZeroMemory(&(players[i]), sizeof(players[i]));

				leaver.whole = htonl(leaver.whole);
				for (int i = 0; i < MAX_PLAYERS; i++)
					if (players[i].id != VACANT_PLAYER)
						send(players[i].waiter, leaver.raw, sizeof(leaver), 0);
				
				numPlayers--;
			}
			else if (activity.type == 3) {
				players[activity.id - 1].xPos = activity.xPos;
				players[activity.id - 1].yPos = activity.yPos;

				union msg mover = formatmsg(3, activity.id, activity.xPos, activity.yPos);
				mover.whole = htonl(mover.whole);
				for (int i = 0; i < MAX_PLAYERS; i++)
					if (players[i].id != VACANT_PLAYER)
						send(players[i].waiter, mover.raw, sizeof(mover), 0);
			}
		}
	}

cleanup:
	// Cleanup
	freeaddrinfo(addr);
	for (int i = 0; i < MAX_PLAYERS; i++)
			closesocket(players[i].waiter);
	closesocket(host);
	WSACleanup();
	return 0;
} 