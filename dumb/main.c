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
// - TODO: Player positions
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib") 

#define MAX_PLAYERS 3
#define VACANT_PLAYER 0
#define CREATE_PLAYERLIST(ptrList)		  \
	struct player list[MAX_PLAYERS];	  \
	for (int i = 0; i < MAX_PLAYERS; i++) \
		list[i].id = VACANT_PLAYER;		  \
	ptrList = list;
#define ADD_PLAYER(ptrList, ptrNewPlayer) \
	for (int i = 0; i < MAX_PLAYERS; i++) {					\
		if (ptrList[i].id == VACANT_PLAYER) {				\
			ptrList[i].sock = ptrNewPlayer->sock;			\
			ptrList[i].addr = ptrNewPlayer->addr;			\
			ptrList[i].id = i + 1;							\
			ptrList[i].x = 0;								\
			ptrList[i].y = 0;								\
			ptrNewPlayer = &ptrList[i];						\
			break;											\
		}													\
	}
	
#define ACCESS_PLAYER   (ptrList, id) ptrList[id - 1]
#define REMOVE_PLAYER   (ptrList, id) ACCESS_PLAYER(ptrList, id).id = VACANT_PLAYER
#define IS_PLAYER_TAKEN (ptrList, id) (int)(ACCESS_PLAYER(ptrList, id).id == VACANT_PLAYER)

#define F8B  11111111
#define BUILD_CONNECTION_STATUS(type, id1, id2, id3) (type << 6) | (id1 << 4) | (id2 << 2) | id3

// Construct player position
// b[0-1]   - player id
// b[2]	    - sign
//	      0 - negative
//		  1 - positive
// b[3-12]  - x coord of player
// b[13]    - sign
//        0 - negative
//        1 - positive
// b[14-23] - y coord of player
#define BUILD_PLAYER_POSITION(id, spx, px, spy, py) (id << 22) | (spx << 21) | (px << 11) | (spy << 10) | py

// Split a 24-bit player position into 3 bytes
#define CHUNK_PLAYER_POSITION(F8B, pp, byte0, byte1, byte2) \
	byte0 = (pp & (0b##F8B << 16)) >> 16;					\
	byte1 = (pp & (0b##F8B << 8 )) >> 8 ;					\
    byte2 =  pp &  0b##F8B 

// Reattach a 24-bit player position that's been split into 3 bytes
#define GLUE_PLAYER_POSITION(byte0, byte1, byte2) (byte0 << 16) | (byte1 << 8) | byte2

#define STRIP_PLAYER_POSITION(pp, id, x, y)  \
	id = (pp & (0b11          << 22)) >> 22; \
	 x = (pp & (0b11111111111 << 11)) >> 11; \
	 y =  pp &  0b11111111111			   ;                 

#define STRIP_COORDINATE(coord, sign, num) \
	sign = coord & 0b100000000000;		   \
	num  = coord & 0b011111111111;		   


// Represents a server-wide or player-specific msg:
// - type: subject of msg describing an event
//   - 0: player join request success (player-specific)
//	 - 1: player joined the server (server-wide)
//   - 2: player left the server (server-wide)
// - target: id of player associated with event
// - id1 + id2: ids of other players (only used with msg type 0)
union out_tcp {
	struct {
		unsigned char type     : 2; 
		unsigned char target   : 2;
		unsigned char id1	   : 2;
		unsigned char id2	   : 2;
	};
	char raw;
};

struct player {
	SOCKET sock;
	struct sockaddr addr;
	int id;
	int x;
	int y;
};

SOCKET host(int type, int proto) {
	int res;
	u_long mode = 1;
	SOCKET sock;
	struct addrinfo* addr;
	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = type;
	hints.ai_protocol = proto;
	hints.ai_flags = AI_PASSIVE;
	res = getaddrinfo(NULL, "3490", &hints, &addr);
	if (res != 0) return res;
	sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (sock == INVALID_SOCKET) { freeaddrinfo(addr); return WSAGetLastError(); }
	if (ioctlsocket(sock, FIONBIO, &mode) == SOCKET_ERROR) { freeaddrinfo(addr); return WSAGetLastError(); }
	if (bind(sock, addr->ai_addr, addr->ai_addrlen) == SOCKET_ERROR) { freeaddrinfo(addr); return WSAGetLastError(); }
	return sock;
}

int main() {
	// Initialize WSA
	WSADATA wsaData;
	int wsainitres = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsainitres != 0) {
		printf("WSAStartup Fail");
		return wsainitres;
	}

	// Initialize host sockets
	SOCKET hostTCP = host(SOCK_STREAM, IPPROTO_TCP);
	SOCKET hostUDP = host(SOCK_DGRAM, IPPROTO_UDP);
	if (listen(hostTCP, SOMAXCONN) == SOCKET_ERROR) WSAGetLastError();

	// Initialize player list
	struct player* players;
	CREATE_PLAYERLIST(players);

	u_long mode = 1;
	int numPlayers = 0;
	int numBytes = 0;
	char bufSendTCP[1];
	char bufRecvTCP[1];
	char bufRecvUDP[3];
	union out_tcp msg;

	while (!(GetAsyncKeyState(VK_END) & 0x01)) {

		// Test 3: Player Validation
		struct player* newPlayer;
		newPlayer->sock = accept(hostTCP, &(newPlayer->addr), NULL);
		if (numPlayers									 == MAX_PLAYERS    || 
			newPlayer->sock								 == INVALID_SOCKET ||
			ioctlsocket(newPlayer->sock, FIONBIO, &mode) == SOCKET_ERROR     ) {

			closesocket(newPlayer->sock);
			goto checkup;
		}
		// Test 3 End

		// Test 4: Add new player
		union out_tcp msg;
		msg.type = 0;
		ADD_PLAYER(players, newPlayer);
		msg.target = newPlayer->id;
		// Populate the rest of the id array indices with the other players (if there are any)
		for (int i = 0, j = 0; i < MAX_PLAYERS; i++) {
			if (players[i].id == newPlayer->id) continue;
			msg.raw |= (players[i].id << (4 + i * 2));
			j++;
		}

		// Notify the new player that they successfully joined
		// And tell them who's already in the server
		bufSendTCP[0] = 
		send(newPlayer.sock, bufSendTCP, 1, 0);
		
		// Notify the other players that a new player joined
		// And tell them who it is
		bufSendTCP[0] = BUILD_CONNECTION_STATUS(1, ids[0], 0, 0);
		for (int i = 0; i < MAX_PLAYERS; i++) 
			if (players[i].id != newPlayer.id && players[i].id != 0) 
				send(players[i].sock, bufSendTCP, 1, 0);

		numPlayers++;
	checkup:
		// Scan for any players who disconnected
		for (int i = 0; i < MAX_PLAYERS; i++) {
			if (players[i].id == 0) continue;
			numBytes = recv(players[i].sock, bufRecvTCP, 1, 0);
			// Secondary condition accounts for players who disconnect by force closing
			if (numBytes == 0 || (numBytes == SOCKET_ERROR && WSAGetLastError() == 10054)) {
				numPlayers--;
				bufSendTCP[0] = BUILD_CONNECTION_STATUS(2, players[i].id, 0, 0);
				closesocket(players[i].sock);
				players[i].id = 0;
				for (int i = 0; i < MAX_PLAYERS; i++) if (players[i].id != 0) send(players[i].sock, bufSendTCP, 1, 0);
			}
		}

		char bufSendUDP[9] = { 0,0,0,0,0,0,0,0,0 };
		int iBufSendUDP = 0;
		for (int i = 0; i < MAX_PLAYERS; i++) {
			if (players[i].id == 0) continue;
			int sx, x, sy, y;
			sx = players[i].x < 0;
			x = abs(players[i].x);
			sy = players[i].y < 0;
			y = abs(players[i].y);

			int pp = BUILD_PLAYER_POSITION(players[i].id, sx, x, sy, y);

			char ppChunks[3];
			CHUNK_PLAYER_POSITION(F8B, pp, ppChunks[0], ppChunks[1], ppChunks[2]);
			bufSendUDP[iBufSendUDP++] = ppChunks[0];
			bufSendUDP[iBufSendUDP++] = ppChunks[1];
			bufSendUDP[iBufSendUDP++] = ppChunks[2];
		}
		for (int i = 0; i < MAX_PLAYERS; i++)
			if (players[i].id != 0)
				sendto(hostUDP, bufSendUDP, 9, 0, &(players[i].addr), sizeof(struct sockaddr));

		numBytes = recvfrom(hostUDP, bufRecvUDP, 3, 0, NULL, NULL);
		if (numBytes != SOCKET_ERROR) {
			int id, x, y;
			STRIP_PLAYER_POSITION( (GLUE_PLAYER_POSITION(bufRecvUDP[0], bufRecvUDP[1], bufRecvUDP[2])) , id, x, y);

			int sx, ax, sy, ay;
			STRIP_COORDINATE(x, sx, ax);
			STRIP_COORDINATE(y, sy, ay);
			players[id - 1].x = ax;
			players[id - 1].y = ay;
			if (sx == 0) players[id - 1].x *= -1;
			if (sy == 0) players[id - 1].y *= -1;
		}
	}
	for (int i = 1; i < MAX_PLAYERS + 1; i++) if (players[i].id != 0) closesocket(players[i].sock);
	closesocket(hostTCP);
	closesocket(hostUDP);
	WSACleanup();
	return 0;
} 