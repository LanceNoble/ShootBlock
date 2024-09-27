#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

int main() {
	int pipe[6];
	u_long mode = 1;
	WSADATA wsaData;
	SOCKET host;
	struct addrinfo* addr;
	struct addrinfo hintsTCP;
	ZeroMemory(&hintsTCP, sizeof(hintsTCP));
	hintsTCP.ai_family = AF_INET;
	hintsTCP.ai_socktype = SOCK_STREAM;
	hintsTCP.ai_protocol = IPPROTO_TCP;

	pipe[0] = WSAStartup(MAKEWORD(2, 2), &wsaData);
	pipe[1] = getaddrinfo(NULL, "30000", &hintsTCP, &addr);

	host = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	pipe[2] = WSAGetLastError();

	ioctlsocket(host, FIONBIO, &mode);
	pipe[3] = WSAGetLastError();

	bind(host, addr->ai_addr, addr->ai_addrlen);
	pipe[4] = WSAGetLastError();

	listen(host, SOMAXCONN);
	pipe[5] = WSAGetLastError();

	for (int i = 0; i < 6; i++) 
		if (pipe[i] != 0) 
			goto end;

	char recvBuf[8];
	int numCli = 0;
	fd_set actives;
	fd_set reads;
	FD_ZERO(&actives);
	const struct timeval timeout = { .tv_sec = 0, .tv_usec = 0 * 1000000 };
	SOCKET waiter;
	while (!(GetAsyncKeyState(VK_ESCAPE) & 0x01)) {
		waiter = accept(host, NULL, NULL);
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
			if (bytes == SOCKET_ERROR && WSAGetLastError() == 10054 || bytes == 0) {	
				FD_CLR(reads.fd_array[i], &actives);
				shutdown(reads.fd_array[i], SD_SEND);
				closesocket(reads.fd_array[i]);
				printf("connected sockets: %i\n", ++numCli);
			}
			if (bytes > 0 && recvBuf[0] == '0') {
				if (send(reads.fd_array[i], "0", 1, 0) == SOCKET_ERROR) {
					printf("%i\n", WSAGetLastError());
				}
				else {
					printf("connected sockets: %i\n", ++numCli);
				}
			}
			if (bytes > 0 && recvBuf[0] == '1') {
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
	freeaddrinfo(addr);
	closesocket(host);
	WSACleanup();
	return 0;
}