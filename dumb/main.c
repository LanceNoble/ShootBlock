#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

int main() {
	int res;
	WSADATA wsaData;
	if (res = WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("WSAStartup fail: %d\n", res);
		return 1;
	}

	struct addrinfo* resAddrTCP = NULL;
	struct addrinfo hintsTCP;
	ZeroMemory(&hintsTCP, sizeof(hintsTCP));
	hintsTCP.ai_family = AF_INET;
	hintsTCP.ai_socktype = SOCK_STREAM;
	hintsTCP.ai_protocol = IPPROTO_TCP;
	res = getaddrinfo(NULL, "30000", &hintsTCP, &resAddrTCP);
	if (res != 0) {
		printf("getaddrinfo TCP fail: %d\n", res);
		WSACleanup();
		return 1;
	}
	struct addrinfo* resAddrUDP = NULL;
	struct addrinfo hintsUDP;
	ZeroMemory(&hintsUDP, sizeof(hintsUDP));
	hintsUDP.ai_family = AF_INET;
	hintsUDP.ai_socktype = SOCK_DGRAM;
	hintsUDP.ai_protocol = IPPROTO_UDP;
	res = getaddrinfo(NULL, "30000", &hintsUDP, &resAddrUDP);
	if (res != 0) {
		printf("getaddrinfo UDP fail: %d\n", res);
		WSACleanup();
		return 1;
	}

	SOCKET sockTCP = socket(resAddrTCP->ai_family, resAddrTCP->ai_socktype, resAddrTCP->ai_protocol);
	if (sockTCP == INVALID_SOCKET) {
		printf("socket TCP fail: %ld\n", WSAGetLastError());
		freeaddrinfo(resAddrTCP);
		WSACleanup();
		return 1;
	}
	SOCKET sockUDP = socket(resAddrUDP->ai_family, resAddrUDP->ai_socktype, resAddrUDP->ai_protocol);
	if (sockUDP == INVALID_SOCKET) {
		printf("socket UDP fail: %ld\n", WSAGetLastError());
		freeaddrinfo(resAddrUDP);
		WSACleanup();
		return 1;
	}

	res = bind(sockTCP, resAddrTCP->ai_addr, resAddrTCP->ai_addrlen);
	if (res == SOCKET_ERROR) {
		printf("bind TCP fail: %d\n", WSAGetLastError());
		freeaddrinfo(resAddrTCP);
		closesocket(sockTCP);
		closesocket(sockUDP);
		WSACleanup();
		return 1;
	}
	freeaddrinfo(resAddrTCP);
	res = bind(sockUDP, resAddrUDP->ai_addr, resAddrUDP->ai_addrlen);
	if (res == SOCKET_ERROR) {
		printf("bind UDP fail: %d\n", WSAGetLastError());
		freeaddrinfo(resAddrUDP);
		closesocket(sockUDP);
		closesocket(sockTCP);
		WSACleanup();
		return 1;
	}

	if (listen(sockTCP, 4) == SOCKET_ERROR) {
		printf("listen fail: %ld\n", WSAGetLastError());
		closesocket(sockTCP);
		closesocket(sockUDP);
		freeaddrinfo(resAddrUDP);
		WSACleanup();
		return 1;
	}

	int i = 0;
	SOCKET clients[4];
	struct sockaddr clientAddrs[4];
	while (i < 4) {
		clients[i] = accept(sockTCP, &clientAddrs[i], NULL);
		if (clients[i] == INVALID_SOCKET) {
			printf("accept TCP fail: %d\n", WSAGetLastError());
			closesocket(sockTCP);
			closesocket(sockUDP);
			freeaddrinfo(resAddrUDP);
			WSACleanup();
			return 1;
		}
		i++;
	}

	char recvbuf[]


	freeaddrinfo(resAddrUDP);
	closesocket(sockUDP);
	closesocket(sockTCP);
	WSACleanup();
	return 0;
}