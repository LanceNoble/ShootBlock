// 10/01/2024
// This program is a server allowing a maximum of 3 client connections
// Its sole purpose is to prompt/help each client sync their data with all other clients AND the server
// The client does the actual syncing
// 
// Client data sync occurs when the client signals the server that something happened on their end
// Server must respond to this signal by relaying it to the rest of the clients
// 
// Server data sync occurs when the server signals the client that something happened on their end
// Client must respond to this signal by changing something on their end
// 
// Data it helps sync so far:
// - List of connected clients (client data)
//   
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib") 

#define MAX_PLAYERS 3

// Construct msg for player
// b[0-1] - msg type
//		  0 (00) - join success
//        1 (01) - new player
//	      2 (10) - player left
// b[2-7] - array of IDs of players in server (2 bits per index)
//		  0 (00) - empty
//		  if b[0-1] == 0 (00): 
//			b[2-3] = New player ID
//			b[4-7] = Other player IDs
//		  if b[0-1] == 1 (01):
//			b[2-3] = New player ID
//			b[4-7] = 0 (00)
//		  if b[0-1] == 2 (10):
//			b[2-3] = ID of player who left
//			b[4-7] = 0 (00)
#define BUILD(type, id0, id1, id2) 0b00000000 | type | (id0 << 2) | (id1 << 4) | (id2 << 6)

#define CONSTRUCT()

void host(int type, int proto, struct addrinfo* addr, SOCKET* sock, int pipe[4]) {
	u_long mode = 1;
	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = type;
	hints.ai_protocol = proto;
	hints.ai_flags = AI_PASSIVE;
	pipe[0] = getaddrinfo(NULL, "3490", &hints, &addr);
	*sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	pipe[1] = WSAGetLastError();
	ioctlsocket(*sock, FIONBIO, &mode);
	pipe[2] = WSAGetLastError();
	bind(*sock, addr->ai_addr, addr->ai_addrlen);
	pipe[3] = WSAGetLastError();
}

int main() {
	// Test 1: Initialize WSA
	WSADATA wsaData;
	int wsainitres = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsainitres != 0) {
		printf("WSAStartup Fail");
		return wsainitres;
	}
	// Test 1 End

	// Test 2: Initialize host socket
	struct addrinfo* addr = NULL;
	SOCKET shost;
	int pipe[4];
	host(SOCK_STREAM, IPPROTO_TCP, addr, &shost, pipe);
	for (int i = 0; i < 4; i++) if (pipe[i] != 0) goto end;
	listen(shost, SOMAXCONN);
	// Test 2 End

	// Bit2-7 in the server's send buffer represent an array of IDs of players in the server
	// That's 6 bits, so 2 bits per ID for a maximum of 3 players that can be in the server
	// A 2bit value of 0 is reserved to represent an empty spot in the server
	// Thus, players MUST have an ID greater than 0 so as not to be confused with an empty server spot
	// IDs are represented via array indices, so we make an array with MAX_PLAYERS + 1 size
	// So that every player can have an ID greater than 0
	SOCKET players[MAX_PLAYERS + 1] = { 0,0,0,0 };

	SOCKET newPlayer;
	u_long mode = 1;
	int numPlayers = 0;
	int numBytes = 0;
	char bufSendTCP[1];
	char bufRecvTCP[1];

	while (!(GetAsyncKeyState(VK_END) & 0x01)) {

		// Test 3: Player Validation
		newPlayer = accept(shost, NULL, NULL);
		if (numPlayers == MAX_PLAYERS || ioctlsocket(newPlayer, FIONBIO, &mode) == SOCKET_ERROR) {
			closesocket(newPlayer);
			goto checkup;
		}
		// Test 3 End

		// Test 4: Add new player
		numPlayers++;
		int ids[MAX_PLAYERS] = { 0,0,0 };

		// Look for an open spot for the new player
		// Remember: Player IDs are represented via array indices
		for (int i = 1; i < MAX_PLAYERS + 1; i++) {
			if (players[i] == 0) { 
				players[i] = newPlayer; 
				ids[0] = i; 
				break; 
			}
		}

		// Populate the rest of the id array indices with the other players (if there are any)
		for (int i = 1, j = 1; i < MAX_PLAYERS + 1; i++) {
			if (players[i] == newPlayer) continue;
			if (players[i] == 0) { ids[j] = 0; j++; }
			if (players[i] != 0) { ids[j] = i; j++; }
		}

		// Notify the new player that they successfully joined
		// And tell them who's already in the server
		bufSendTCP[0] = BUILD(0, ids[0], ids[1], ids[2]);
		send(newPlayer, bufSendTCP, 1, 0);
		
		// Notify the other players that a new player joined
		// And tell them who it is
		bufSendTCP[0] = BUILD(1, ids[0], 0, 0);
		for (int i = 1; i < MAX_PLAYERS + 1; i++) if (players[i] != newPlayer && players[i] != 0) send(players[i], bufSendTCP, 1, 0);

	checkup:
		// Scan for any players who disconnected
		for (int i = 1; i < MAX_PLAYERS + 1; i++) {
			if (players[i] == 0) continue;
			numBytes = recv(players[i], bufRecvTCP, 1, 0);
			// Secondary condition accounts for players who disconnect by force closing
			if (numBytes == 0 || (numBytes == SOCKET_ERROR && WSAGetLastError() == 10054)) {
				numPlayers--;
				closesocket(players[i]);
				players[i] = 0;
				bufSendTCP[0] = BUILD(2, i, 0, 0);
				for (int i = 1; i < MAX_PLAYERS + 1; i++) if (players[i] != 0) send(players[i], bufSendTCP, 1, 0);
			}
		}
	}
	for (int i = 1; i < MAX_PLAYERS + 1; i++) if (players[i] != 0) closesocket(players[i]);
end:
	freeaddrinfo(addr);
	closesocket(shost);
	WSACleanup();
	return 0;
}