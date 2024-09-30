/*
Flowchart:
1. Player TCP connects to server
2. Upon receiving connection request, server accepts and checks following conditions
   - If address is in list of players in server
	  - Send player already connected msg
   - If address not in list of players in server, add them, start UDP msging them, and listen to their UDP msgs
	  - Add them to list
	  - Send all players id list msg
3. Player TCP closesocket() from server
4. Upon receiving 

3. 

3. 
3. 
1. Player TCP requests to join server
2. Upon receiving request, server checks following conditions:
2. Server checks player address
   If address matches address in list of players in server, 
   1. Join server
   2. (If req 1 succeeds) request IDs of all players in the server
2. Server 
2. Player TCP disconnects (flowchart stops here if request 1 failed)
3. 
3. Player UDP continuously notifies server that they did something
   Server receives notif and continuously sends all players that same notif


3. Upon game event trigger, server continuously sends all players UDP notifs of that event
   Upon receiving notif, player syncs with event, then continuously UDP notifies server that they synced
   Upon 
   Player continuously receives UDP notifs of events from server
   Player syncs with event, then continuously UDP notifies server that they handled event

   Player stops UDP notifying after not receiving that specific UDP notif for a certain period of time

3. Server continuously UDP notifies all players (except the one who just joined) in the server that a player joined
   Player syncs with this event, then continuously UDP notifies server that they handled this event
   Server receives notification and promptly stops notifying ONLY this player
   Player stops UDP notifying after 
*/
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#pragma comment(lib, "Ws2_32.lib") 

#define NUM_BYTES_SENDBUF_TCP 1
#define NUM_BYTES_RECVBUF_TCP 1
#define NUM_BYTES_SENDBUF_UDP 17
#define NUM_BYTES_RECVBUF_UDP 1

#define MAX_NUM_PLAYERS 4
#define NUM_BYTES_PLAYERNAME 16

#define STRIP(by, mask, start) (by & 0b##mask) >> start

// when a player wants to join, they will make 2 requests before shutting down the tcp connection
// request 1: join
// if request 1 succeeds:
// request 2: fetch player ids
// shutdown tcp connection

// when a player joins or leaves, all players in the server (except the one who just joined/left)
// will periodically get a udp message saying that that player joined/left
// if the player wants to stop getting these messages they must sync their game with the event that just happened
// once they do that, they make a tcp request 

// Deconstruct a message describing a player's request
// bit0-1 - request type
//		0 - join server
//      1 - fetch player ids
//		2 - leave server
// bit2-4 - the player's id (required to leave server)
//		0 - empty
// bit5-7 - padding
#define STRIP_TCP

// Construct a message responding to a player's request
// bit0-0  - client request status
//		0  - success
//		1  - fail
// bit1-3  - the player's id
//      0  - empty
// bit4-15 - list of player ids (3 bits each)
//		0  - open player spot
#define BUILD_TCP(type, id) type | (id << 1);

// Construct a message notifying a player of a game event
// bit0-0 - event type
//		0 - a player has joined
//		1 - a player has left
// bit1-3 - id of the player associated with the event
// bit4-7 - padding
#define BUILD_UDP

// Deconstruct a message describing a player's status
// bit0-1 - sync status
//		0 - the player has synced with the join event
//		1 - the player has synced with the leave event
//		2 - the player is alive
// bit2-4 - id of the player associated with the event
// bit5-7 - padding
#define STRIP_UDP



// TODO:
// have a queue of players who left and 
// have a queue of players who joined 
// and don't remove them from the queue until
// all clients have notified the server that they handled the change
// TODO:
// Check for max player count
// TODO: 
// our client-server's TCP connections are temporary
// so the server won't know if the client disconnects by force closing the program instead of pressing the leave button
// we could check if the client hasn't sent a message in a while, but they could just be afk
// therefore, implement a heartbeat system where the client must send a udp message indicating they're alive every fixed interval
// TODO: 
// allocate bits specifying the message's intended length so client knows if the whole message was sent or not
// 







// client can have two sockets
struct node {
	char ip [NUM_BYTES_PLAYERNAME];
	struct sockaddr addr;
	struct node* next;
};

int addrSize = sizeof(struct sockaddr);
u_long mode = 1;
SOCKET hostTCP;
SOCKET hostUDP;
struct addrinfo* addrTCP;
struct addrinfo* addrUDP;

void freecliaddr() {

}

void hsock(int type, int proto, struct addrinfo* addr, SOCKET* sock, int pipe[5]) {
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
	
	int pipeTCP[4];
	int pipeUDP[4];
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 1;

	hsock(SOCK_STREAM, IPPROTO_TCP, addrTCP, &hostTCP, pipeTCP);
	hsock(SOCK_DGRAM, IPPROTO_UDP, addrUDP, &hostUDP, pipeUDP);
	for (int i = 0; i < 4; i++) if (pipeTCP[i] != 0 || pipeUDP[i] != 0) goto end;
	listen(hostTCP, SOMAXCONN);

	fd_set reads;
	FD_ZERO(&reads);
	const struct timeval timeout = { .tv_sec = 0, .tv_usec = 0 * 1000000 };

	int numClis = 0;
	struct node* head = malloc(sizeof(struct node));
	struct sockaddr* guestAddrRaw = malloc(sizeof(struct sockaddr));
	if (head == NULL || guestAddrRaw == NULL) return 1;
	struct node* tail = head;
	
	while (!(GetAsyncKeyState(VK_ESCAPE) & 0x01)) {
		SOCKET waiter = accept(hostTCP, NULL, NULL);
		if (ioctlsocket(waiter, FIONBIO, &mode) != SOCKET_ERROR) FD_SET(waiter, &reads);
		if (reads.fd_count == 0) continue;

		select(0, &reads, NULL, NULL, &timeout);
		for (u_int i = 0; i < reads.fd_count; i++) {
			char bufSendTCP[1];
			char bufRecvTCP[1];
			int numBytes = recv(reads.fd_array[i], bufRecvTCP, 1, 0);
			int type = STRIP(bufRecvTCP[0], 00000000, 0);
			int id = STRIP(bufRecvTCP[0], 00001110, 1);
			if (numBytes == 0 || (numBytes == SOCKET_ERROR && WSAGetLastError() == 10054)) {
				FD_CLR(reads.fd_array[i], &reads);
				shutdown(reads.fd_array[i], SD_SEND);
				closesocket(reads.fd_array[i]);
			}
			else if (type == 0) {
				if (numClis == MAX_NUM_PLAYERS) {
					bufSendTCP[0] = BUILD_TCP(0, 0);
					goto response;
				}
				bufSendTCP[0] = BUILD_TCP(0, ++numClis);
				getpeername(reads.fd_array[i], guestAddrRaw, &addrSize);
				inet_ntop(AF_INET, &((struct sockaddr_in*)guestAddrRaw)->sin_addr, tail->ip, 16);
				printf("%s joined\nThere are now %i players\n\n", tail->ip, numClis);
				tail->addr = *guestAddrRaw;
				tail->next = malloc(sizeof(struct node));
				if (tail->next == NULL) return 1;
				tail = tail->next;
			response:
				send(reads.fd_array[i], bufSendTCP, 1, 0);
			}
			else if (type == 1) {
				bufSendTCP[0] = BUILD_TCP(1, numClis - 1);
				if (send(reads.fd_array[i], bufSendTCP, 1, 0) == SOCKET_ERROR) continue;
				char ipBuf[16];
				char* ip;
				getpeername(reads.fd_array[i], guestAddrRaw, &addrSize);
				ip = inet_ntop(AF_INET, &((struct sockaddr_in*)guestAddrRaw)->sin_addr, ipBuf, 16);
				for (int i = 0; i < 4; i++) {
					/*
					if (strcmp(ip, addrs[i]) == 0) {
						printf("%s left\n%i players remain\n\n", addrs[i], numClis);
						ZeroMemory(addrs[i], 16);
						numClis--;
						break;
					}
					*/
					
				}
			}
		}

		char bufRecvUDP;
		char bufSendUDP[17];


		struct node* i = head;
		while (i != NULL) {
			
			//sendto(hostUDP,);
		}
	}
	for (u_int i = 0; i < reads.fd_count; i++) {
		shutdown(reads.fd_array[i], SD_SEND);
		closesocket(reads.fd_array[i]);
	}
	FD_ZERO(&reads);
end:
	freeaddrinfo(addrTCP);
	closesocket(hostTCP);
	WSACleanup();
	return 0;
}