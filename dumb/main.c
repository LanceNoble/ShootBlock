#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#pragma comment(lib, "Ws2_32.lib") 

#define MAX_PLAYERS 4
#define STRIP(by, mask, start) (by & 0b##mask) >> start
// TODO:
// Check for max player count
// TODO: 
// our client-server's TCP connections are temporary
// so the server won't know if the client disconnects by force closing the program instead of pressing the leave button
// we could check if the client hasn't sent a message in a while, but they could just be afk
// therefore, implement a heartbeat system where the client must send a udp message indicating they're alive every fixed interval
// TODO: 
// allocate bits specifying the message's intended length so client knows if the whole message was sent or not
// Construct a message responding to a client's TCP request
// bit0-0 - the client's request
//		0 - request to join the server
//		1 - request to leave the server
// bit1-3 - the client's id
//		0 - request fail
// bit4-7 - padding
#define BUILD_TCP(type, id) type | (id << 1);

u_long mode = 1;
SOCKET hostTCP;
SOCKET hostUDP;
struct addrinfo* addrTCP;
struct addrinfo* addrUDP;

struct node {
	int hi;
};
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
	char addrs[4][16];
	
	while (!(GetAsyncKeyState(VK_ESCAPE) & 0x01)) {
		struct sockaddr_in guestAddr;
		SOCKET waiter = accept(hostTCP, NULL, NULL);
		if (ioctlsocket(waiter, FIONBIO, &mode) != SOCKET_ERROR) FD_SET(waiter, &reads);
		if (reads.fd_count == 0) continue;

		select(0, &reads, NULL, NULL, &timeout);
		for (int i = 0; i < reads.fd_count; i++) {
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
				if (numClis == MAX_PLAYERS) {
					bufSendTCP[0] = BUILD_TCP(0, 0);
					send(reads.fd_array[i], bufSendTCP, 1, 0);
					continue;
				}
				bufSendTCP[0] = BUILD_TCP(0, ++numClis);
				/*
				* for (int i = 0; ) {

				}
				*/
				
				send(reads.fd_array[i], bufSendTCP, 1, 0);
				getpeername(reads.fd_array[i], &guestAddr, sizeof(struct sockaddr));
				inet_ntop(AF_INET, &guestAddr.sin_addr, addrs[numClis], 16);
				printf("%s joined\n%i players remain\n\n", addrs[numClis], numClis);
			}
			else if (type == 1) {
				bufSendTCP[0] = BUILD_TCP(1, numClis - 1);
				if (send(reads.fd_array[i], bufSendTCP, 1, 0) == SOCKET_ERROR) continue;
				char ipBuf[16];
				char* ip;
				getpeername(reads.fd_array[i], &guestAddr, sizeof(struct sockaddr));
				ip = inet_ntop(AF_INET, &guestAddr.sin_addr, ipBuf, 16);
				for (int i = 0; i < 4; i++) {
					if (strcmp(ip, addrs[i]) == 0) {
						printf("%s left\n%i players remain\n\n", addrs[i], numClis);
						ZeroMemory(addrs[i], 16);
						numClis--;
						break;
					}
				}
			}
		}
	}
	for (int i = 0; i < reads.fd_count; i++) {
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