#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

u_long mode = 1;
SOCKET hostTCP;
SOCKET hostUDP;
struct addrinfo* addrTCP;
struct addrinfo* addrUDP;
void hsock(int type, int proto, struct addrinfo* addr, SOCKET* sock, int pipe[5]) {
	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = type;
	hints.ai_protocol = proto;
	hints.ai_flags = AI_PASSIVE;
	pipe[0] = getaddrinfo(NULL, "3490", &hints, &addr);
	sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	pipe[1] = WSAGetLastError();
	ioctlsocket(sock, FIONBIO, &mode);
	pipe[2] = WSAGetLastError();
	bind(sock, addr->ai_addr, addr->ai_addrlen);
	pipe[3] = WSAGetLastError();
}
int main() {
	int pipeTCP[4];
	int pipeUDP[4];
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) goto end;
	hsock(SOCK_STREAM, IPPROTO_TCP, addrTCP, hostTCP, pipeTCP);
	hsock(SOCK_DGRAM, IPPROTO_UDP, addrUDP, hostUDP, pipeUDP);
	for (int i = 0; i < 4; i++) if (pipeTCP[i] != 0 || pipeUDP[i] != 0) goto end;
	listen(hostTCP, SOMAXCONN);
	
	char recvBuf[8];
	int numCli = 0;
	int maxNumCli = 3;
	fd_set actives;
	fd_set reads;
	FD_ZERO(&actives);
	const struct timeval timeout = { .tv_sec = 0, .tv_usec = 0 * 1000000 };
	SOCKET waiter;
	while (!(GetAsyncKeyState(VK_ESCAPE) & 0x01)) {
		waiter = accept(hostTCP, NULL, NULL);
		if (ioctlsocket(waiter, FIONBIO, &mode) == SOCKET_ERROR)
			goto activityCheck;
		/*
		if (actives.fd_count == 4) {
			shutdown(waiter, SD_SEND);
			closesocket(waiter);
			goto activityCheck;
		}
		*/
		
		FD_SET(waiter, &actives);


	activityCheck:
		if (actives.fd_count == 0)
			continue;
		
		FD_ZERO(&reads);
		for (int i = 0; i < actives.fd_count; i++)
			FD_SET(actives.fd_array[i], &reads);
		
		select(0, &reads, NULL, NULL, &timeout);
		for (int i = 0; i < reads.fd_count; i++) {
			int bytes = recv(reads.fd_array[i], recvBuf, 8, 0);
			// Account for people who abort program
			if (bytes == SOCKET_ERROR && WSAGetLastError() == 10054) {	
				FD_CLR(reads.fd_array[i], &actives);
				shutdown(reads.fd_array[i], SD_SEND);
				closesocket(reads.fd_array[i]);
				printf("connected sockets: %i\n", --numCli);
			}
			if (bytes == 1 && recvBuf[0] == '0') {
				if (send(reads.fd_array[i], "0", 1, 0) == SOCKET_ERROR) {
					printf("%i\n", WSAGetLastError());
				}
				else {
					printf("connected sockets: %i\n", ++numCli);
				}
			}
			if (bytes == 1 && recvBuf[0] == '1') {
				if (send(reads.fd_array[i], "1", 1, 0) == SOCKET_ERROR) {
					printf("%i\n", WSAGetLastError());
				}
				else {
					printf("connected sockets: %i\n", --numCli);
				}
			}
		}


		/*
		for (int i = 0; i < actives.fd_count; i++) {
			clientCount[0] = actives.fd_count;
			if (send(actives.fd_array[i], clientCount, 1, 0) == SOCKET_ERROR) {
				printf("%i\n", WSAGetLastError());
			}
		}
		*/
	}
	for (int i = 0; i < actives.fd_count; i ++) {
		shutdown(actives.fd_array[i], SD_SEND);
		closesocket(actives.fd_array[i]);
	}
	FD_ZERO(&actives);
	FD_ZERO(&reads);
end:
	freeaddrinfo(addrTCP);
	closesocket(hostTCP);
	WSACleanup();
	return 0;
}