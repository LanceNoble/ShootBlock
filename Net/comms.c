#include "comms.h"

#include <winsock2.h>
#include <ws2tcpip.h>

enum End {
	UNKNOWN = -1,
	LIL = 0,
	BIG = 1
};

static enum End end = UNKNOWN;
static WSADATA* wsaData;

// Reverse a contiguous set of bytes for sending and receiving network data
// Does not check to see if the bytes have already been flipped
void flip(unsigned char* const bytes, const unsigned short sz) {
	if (sz == 1) {
		return;
	}
	if (end == UNKNOWN) {
		unsigned short test = 0x1234;
		unsigned char* first = (unsigned char*)(&test);
		if (*first == 0x34) {
			end = LIL;
		}
		else {
			end = BIG;
		}
	}
	if (end == BIG) {
		return;
	}
	unsigned short i = 0;
	unsigned short j = sz - 1;
	while (i < j) {
		unsigned char temp = bytes[i];
		bytes[i] = bytes[j];
		bytes[j] = temp;
		i++;
		j--;
	}
}

// Enable a program to use windows sockets
short wsa_create() {
	if (wsaData != NULL) {
		return 0;
	}
	wsaData = malloc(sizeof(WSADATA));
	if (wsaData == NULL) {
		return -1;
	}
	return WSAStartup(MAKEWORD(2, 2), wsaData);
}

// Clean up the resources used to initialize windows sockets
unsigned short wsa_destroy() {
	if (wsaData == NULL) {
		return 0;
	}
	if (WSACleanup() == SOCKET_ERROR) {
		return WSAGetLastError();
	}
	free(wsaData);
	wsaData = NULL;
	return 0;
}
