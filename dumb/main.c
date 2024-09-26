

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

int main() {
	int res;
	WSADATA wsaData;
	if (res = WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("WSAStartup fail\n");
		return res;
	}
	struct addrinfo* addr = NULL;
	struct addrinfo hintsTCP;
	ZeroMemory(&hintsTCP, sizeof(hintsTCP));
	hintsTCP.ai_family = AF_INET;
	hintsTCP.ai_socktype = SOCK_STREAM;
	hintsTCP.ai_protocol = IPPROTO_TCP;
	if (res = getaddrinfo(NULL, "30000", &hintsTCP, &addr) != 0) {
		printf("getaddrinfo fail\n");
		WSACleanup();
		return res;
	}
	SOCKET host = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (host == INVALID_SOCKET) {
		printf("socket fail\n");
		freeaddrinfo(addr);
		WSACleanup();
		return WSAGetLastError();
	}
	u_long mode = 1;
	if (ioctlsocket(host, FIONBIO, &mode) == SOCKET_ERROR) {
		printf("ioctlsocket fail\n");
		closesocket(host);
		freeaddrinfo(addr);
		WSACleanup();
		return WSAGetLastError();
	}
	if (bind(host, addr->ai_addr, addr->ai_addrlen) == SOCKET_ERROR) {
		printf("bind fail\n");
		freeaddrinfo(addr);
		closesocket(host);
		WSACleanup();
		return WSAGetLastError();
	}
	freeaddrinfo(addr);
	if (listen(host, 4) == SOCKET_ERROR) {
		printf("listen fail");
		closesocket(host);
		WSACleanup();
		return WSAGetLastError();
	}

	fd_set actives;
	FD_ZERO(&actives);
	FD_SET(host, &actives);
	fd_set reads;
	const struct timeval timeout = { .tv_sec = 0, .tv_usec = 0 * 1000000 };
	while (1) {
		
		SOCKET newSock = accept(host, NULL, NULL);
		if (newSock != INVALID_SOCKET && actives.fd_count < 5) {
			FD_SET(newSock, &actives);
			
			for (int i = 0; i < actives.fd_count; i++) {
				send(actives.fd_array[i], "j", 1, 0);
			}
			
			
		}

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
				shutdown(reads.fd_array[i], SD_SEND);
				closesocket(reads.fd_array[i]);
				FD_CLR(reads.fd_array[i], &actives);
			}
			
			if (bytes == 1) {
				for (int i = 0; i < actives.fd_count; i++)
					send(actives.fd_array[i], buf, 1, 0);
			}
			
			
		}
		printf("connected sockets: %i\n", actives.fd_count - 1);
	}
	for (int i = 0; i < 5; i ++) {
		//shutdown(reads[i], SD_SEND);
		//closesocket(reads[i]);
	}
	closesocket(host); 
	WSACleanup();
	return 0;
}