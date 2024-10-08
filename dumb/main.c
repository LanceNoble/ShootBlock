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

// Represents a server-wide or player-specific msg:
// - type: subject of msg describing an event
//   - 0: player join request success (player-specific)
//	 - 1: player joined the server (server-wide)
//   - 2: player left the server (server-wide)
// - target: id of player associated with event
// - id1 + id2: ids of other players (only used with msg type 0)
union tcp_msg {
	struct {
		unsigned char type     : 2; 
		unsigned char target   : 2;
		unsigned char ids	   : 4;
	};
	char raw[1];
};

union udp_msg {
	struct {
		unsigned int id : 2;
		signed   int x  : 15;
		signed   int y  : 15;
	};
	int whole;
	char raw[4];
};

struct player {
	SOCKET waiter;
	unsigned int id;
	signed int x;
	signed int y;
	struct sockaddr udpAddr;
};

int host(int type, int proto, SOCKET* shost) {
	int res;
	u_long mode = 1;
	struct addrinfo* addr;
	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = type;
	hints.ai_protocol = proto;
	hints.ai_flags = AI_PASSIVE;
	res = getaddrinfo(NULL, "3490", &hints, &addr);
	if (res != 0) 
		return res;
	*shost = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (*shost == INVALID_SOCKET) {
		freeaddrinfo(addr); 
		return WSAGetLastError(); 
	}
	if (ioctlsocket(*shost, FIONBIO, &mode) == SOCKET_ERROR) {
		freeaddrinfo(addr); 
		return WSAGetLastError(); 
	}
	if (bind(*shost, addr->ai_addr, addr->ai_addrlen) == SOCKET_ERROR) {
		freeaddrinfo(addr); 
		return WSAGetLastError(); 
	}
	freeaddrinfo(addr);
	return 0;
}

int main() {
	int status;

	// Initialize WSA
	WSADATA wsaData;
	status = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (status != 0) {
		printf("WSAStartup Fail");
		return status;
	}

	// Initialize host sockets
	SOCKET hostTCP;
	status = host(SOCK_STREAM, IPPROTO_TCP, &hostTCP);
	if (status != 0)
		return status;

	if (listen(hostTCP, SOMAXCONN) == SOCKET_ERROR)
		return WSAGetLastError();

	SOCKET hostUDP; 
	status = host(SOCK_DGRAM, IPPROTO_UDP, &hostUDP);
	if (status != 0)
		return status;

	// Initialize player list
	struct player players[MAX_PLAYERS];
	ZeroMemory(players, sizeof(*players) * MAX_PLAYERS);

	u_long mode = 1;
	int numPlayers = 0;

	while (!(GetAsyncKeyState(VK_END) & 0x01)) {
		// Test 3: Player Validation
		if (numPlayers == MAX_PLAYERS)
			goto checkup;

		SOCKET waiter = accept(hostTCP, NULL, NULL);
		if (waiter == INVALID_SOCKET)
			goto checkup;

		if (ioctlsocket(waiter, FIONBIO, &mode) == SOCKET_ERROR) {
			closesocket(waiter);
			goto checkup;
		}

		struct player* next = NULL;
		for (int i = 0; i < MAX_PLAYERS; i++) {
			if (players[i].id == VACANT_PLAYER) {
				next = &(players[i]);
				next->waiter = waiter;
				next->id = i + 1;
				next->x = 0;
				next->y = 0;
				break;
			}
		}
		if (next == NULL) {
			closesocket(waiter);
			goto checkup;
		}
		// Test 3 End

		union tcp_msg preData;
		preData.type = 0;
		preData.target = next->id;
		preData.ids = 0;
		for (int i = 0, j = 0; i < MAX_PLAYERS; i++) {
			if (players[i].id == next->id) continue;
			preData.ids |= (players[i].id << (2 * j));
			j++;
		}
		send(next->waiter, preData.raw, sizeof(preData), 0);

		union tcp_msg newData;
		newData.type = 1;
		newData.target = next->id;
		newData.ids = 0;
		for (int i = 0; i < MAX_PLAYERS; i++) 
			if (players[i].id != next->id && 
				players[i].id != VACANT_PLAYER) 
				send(players[i].waiter, newData.raw, sizeof(newData), 0);

		numPlayers++;
	checkup:
		// Scan for any players who disconnected
		for (int i = 0; i < MAX_PLAYERS; i++) {
			if (players[i].id == VACANT_PLAYER) 
				continue;

			union tcp_msg oldData;
			int numBytes = recv(players[i].waiter, oldData.raw, sizeof(oldData), 0);

			// Secondary condition accounts for players who disconnect by force closing
			if (numBytes == 0 || 
				(numBytes == SOCKET_ERROR && WSAGetLastError() == 10054)) {

				oldData.type = 2;
				oldData.target = players[i].id;
				oldData.ids = 0;

				closesocket(players[i].waiter);
				ZeroMemory(&(players[i]), sizeof(players[i]));

				for (int i = 0; i < MAX_PLAYERS; i++) 
					if (players[i].id != VACANT_PLAYER) 
						send(players[i].waiter, oldData.raw, sizeof(oldData), 0);

				numPlayers--;
			}
		}

		union udp_msg idealPos;
		struct sockaddr udpAddr;
		int udpAddrLen = sizeof(udpAddr);
		int numBytes = recvfrom(hostUDP, idealPos.raw, sizeof(idealPos), 0, &udpAddr, &udpAddrLen);

		if (numBytes == SOCKET_ERROR)
			continue;
		
		idealPos.whole = ntohl(idealPos.whole);

		if (players[idealPos.id - 1].udpAddr.sa_family == 0)
			players[idealPos.id - 1].udpAddr = udpAddr;

		players[idealPos.id - 1].x = idealPos.x;
		players[idealPos.id - 1].y = idealPos.y;

		/*
		idealPos.whole = htonl(idealPos.whole);
		for (int i = 0; i < MAX_PLAYERS; i++)
			if (players[i].id != VACANT_PLAYER && 
				players[i].udpAddr.sa_family != 0 &&
				players[i].id != idealPos.id)
				sendto(hostUDP, idealPos.raw, sizeof(idealPos), 0, &(players[i].udpAddr), sizeof(players[i].udpAddr));
		*/

		for (int i = 0; i < MAX_PLAYERS; i++) {
			if (players[i].id == VACANT_PLAYER)
				continue;
			for (int j = 0; j < MAX_PLAYERS; j++) {
				if (players[j].id == VACANT_PLAYER || players[j].id == players[i].id)
					continue;
				union udp_msg pos;
				pos.id = players[j].id;
				pos.x = players[j].x;
				pos.y = players[j].y;
				pos.whole = htonl(pos.whole);
				sendto(hostUDP, pos.raw, sizeof(pos), 0, &(players[i].udpAddr), sizeof(players[i].udpAddr));
			}
		}
	}
	for (int i = 0; i < MAX_PLAYERS; i++) if (players[i].id != VACANT_PLAYER) closesocket(players[i].waiter);
	closesocket(hostTCP);
	closesocket(hostUDP);
	WSACleanup();
	return 0;
} 