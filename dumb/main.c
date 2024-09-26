#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

int main() {
	int pipe[6];
	int res;
	u_long mode = 1;
	WSADATA wsaData;
	SOCKET host;
	struct addrinfo* addr;
	struct addrinfo hintsTCP;
	ZeroMemory(&hintsTCP, sizeof(hintsTCP));
	hintsTCP.ai_family = AF_INET;
	hintsTCP.ai_socktype = SOCK_STREAM;
	hintsTCP.ai_protocol = IPPROTO_UDP;

	pipe[0] = WSAStartup(MAKEWORD(2, 2), &wsaData);
	pipe[1] = getaddrinfo(NULL, "30000", &hintsTCP, &addr);

	host = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	pipe[2] = WSAGetLastError();

	ioctlsocket(host, FIONBIO, &mode);
	pipe[3] = WSAGetLastError();

	bind(host, addr->ai_addr, addr->ai_addrlen);
	pipe[4] = WSAGetLastError();

	listen(host, 4);
	pipe[5] = WSAGetLastError();

	for (int i = 0; i < 6; i++) 
		if (pipe[i] != 0) 
			goto end;

	char clientCount[32] = "0";
	fd_set actives;
	FD_ZERO(&actives);
	fd_set reads;
	const struct timeval timeout = { .tv_sec = 0, .tv_usec = 0 * 1000000 };
	while (1) {
		if (actives.fd_count < 4) {
			SOCKET newSock = accept(host, NULL, NULL);
			if (ioctlsocket(newSock, FIONBIO, &mode) != SOCKET_ERROR) {
				FD_SET(newSock, &actives);
				for (int i = 0; i < actives.fd_count; i++) {
					if (actives.fd_array[i] != host) {
						clientCount[0]++;
						if (send(actives.fd_array[i], clientCount, 32, 0) == SOCKET_ERROR) {
							printf("%i\n", WSAGetLastError());
						}
					}
				}
			}
		}

		if (actives.fd_count == 0)
			continue;
		FD_ZERO(&reads);
		for (int i = 0; i < actives.fd_count; i++)
			FD_SET(actives.fd_array[i], &reads);
		
		select(0, &reads, NULL, NULL, &timeout);
		for (int i = 0; i < reads.fd_count; i++) {
			if (reads.fd_array[i] == host)
				continue;

			char buf[64];
			int bytes = recv(reads.fd_array[i], buf, 64, 0);
			if (bytes == SOCKET_ERROR && WSAGetLastError() == 10054 || bytes == 0) {	
				FD_CLR(reads.fd_array[i], &actives);
				shutdown(reads.fd_array[i], SD_SEND);
				closesocket(reads.fd_array[i]);
				for (int i = 0; i < actives.fd_count; i++) {
					if (actives.fd_array[i] != host) {
						clientCount[0]--;
						send(actives.fd_array[i], clientCount, 32, 0);
					}
				}
			}
		}
		printf("connected sockets: %i\n", actives.fd_count - 1);
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