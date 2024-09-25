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

	int iReads = 0;
	int iWrites = 0;
	SOCKET reads[5];
	SOCKET writes[5];
	reads[0] = host;
	writes[0] = host;
	fd_set readfds = { .fd_count = 5,.fd_array = reads };
	fd_set writefds = { .fd_count = 5,.fd_array = writes };
	const struct timeval timeout = { .tv_sec = 60, .tv_usec = 60000000 };
	while (iReads < 5) {
		SOCKET sock;
		select(0, &readfds, &writefds, NULL, &timeout);
		if (sock = accept(host, NULL, NULL) != INVALID_SOCKET && FD_ISSET(host, &readfds) && iReads < 5) {
			reads[iReads] = sock;
			iReads++;
		}
		printf("connected sockets: %i\n", iReads);
	}
	for (int i = 0; i < 5; i ++) {
		shutdown(reads[i], SD_SEND);
		closesocket(reads[i]);
	}
	closesocket(host); 
	WSACleanup();
	return 0;
}