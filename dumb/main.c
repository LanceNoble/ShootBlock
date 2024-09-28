/*
cli tcp msg (5 bytes):
bi0     = type
bi1-6   = bodyLength
bi7-38  = body
bi39-40 = padding

join:
bi0     = 0
bi1-6   = 100000
bi7-38  = ip
bi39-40 = 00

leave:
bi0     = 1
bi1-6   = 000100
bi7-38  = 000...id
bi39-40 = 00

convert msg to bin:
1. 
*/
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

u_long mode = 1;
SOCKET hostTCP;
SOCKET hostUDP;
struct addrinfo* addrTCP;
struct addrinfo* addrUDP;

struct node {
	char* ip;
	struct node* next;
};
void dtob(char c, char b[9]) {
	int i = 0;
	int j = 7;
	for (int i = 0; i < 8; i++) b[i] = '0';
	while (c != 0) {
		if (c % 2 == 0) b[i] = '0';
		else b[i] = '1';
		c /= 2;
		i++;
	}
	i = 0;
	while (j > i) {
		b[j] = b[j] ^ b[i];
		b[i] = b[j] ^ b[i];
		b[j] = b[j] ^ b[i];
		i++;
		j--;
	}
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
	for (int i = 0; i < 32; i++) {
		char b[9];
		b[8] = '\0';
		dtob(i, b);
		printf("%i = %s\n", i, b);
	}
	exit(1);
	char** clis = calloc(2048, sizeof(char*));
	if (clis == NULL) {
		printf("malloc fail\n");
		return 1;
	}
	int pipeTCP[4];
	int pipeUDP[4];
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) goto end;
	hsock(SOCK_STREAM, IPPROTO_TCP, addrTCP, &hostTCP, pipeTCP);
	hsock(SOCK_DGRAM, IPPROTO_UDP, addrUDP, &hostUDP, pipeUDP);
	for (int i = 0; i < 4; i++) if (pipeTCP[i] != 0 || pipeUDP[i] != 0) goto end;
	listen(hostTCP, SOMAXCONN);
	fd_set reads;
	FD_ZERO(&reads);
	const struct timeval timeout = { .tv_sec = 0, .tv_usec = 0 * 1000000 };
	struct sockaddr_in newGuest;
	int numCli = 0;
	char recvBuf[4];
	SOCKET waiter;
	while (!(GetAsyncKeyState(VK_ESCAPE) & 0x01)) {
		waiter = accept(hostTCP, &newGuest, NULL);
		if (ioctlsocket(waiter, FIONBIO, &mode) == SOCKET_ERROR) goto activityCheck;
		FD_SET(waiter, &reads);
	activityCheck:
		if (reads.fd_count == 0)
			continue;
		select(0, &reads, NULL, NULL, &timeout);
		for (int i = 0; i < reads.fd_count; i++) {
			int bytes = recv(reads.fd_array[i], recvBuf, 4, 0);
			if (bytes == 1 && recvBuf[0] == '0' && send(reads.fd_array[i], "0", 1, 0) != SOCKET_ERROR) {
				char ipBuf[16];
				char* ip;
				struct sockaddr_in addr;
				getpeername(reads.fd_array[i], &addr, sizeof(struct sockaddr));
				ip = inet_ntop(AF_INET, &addr.sin_addr, &ipBuf, 16);
				clis[reads.fd_array[i]] = ip;
				printf("connected sockets: %i\n", ++numCli);
			}
			else if (bytes == 1 && recvBuf[0] == '1' && send(reads.fd_array[i], "1", 1, 0) != SOCKET_ERROR) {
				printf("connected sockets: %i\n", --numCli);
			}
			else if (bytes == 0)
				FD_CLR(reads.fd_array[i], &reads);
			// For people who send a TCP request then end program before recv
			else if (bytes == SOCKET_ERROR && WSAGetLastError() == 10054) {	
				
			}
		removeCli:
			printf("Client %s disconnected\n", clis[reads.fd_array[i]]);
			printf("There are now %i clients\n", --numCli);
			clis[reads.fd_array[i]] = NULL;
			FD_CLR(reads.fd_array[i], &reads);
			shutdown(reads.fd_array[i], SD_SEND);
			closesocket(reads.fd_array[i]);
		}
	}
	for (int i = 0; i < reads.fd_count; i++) {
		shutdown(reads.fd_array[i], SD_SEND);
		closesocket(reads.fd_array[i]);
	}
	FD_ZERO(&reads);
end:
	free(clis);
	freeaddrinfo(addrTCP);
	closesocket(hostTCP);
	WSACleanup();
	return 0;
}