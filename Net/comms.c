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

void flip_bytes(char* const bytes, const unsigned short sz) {
	if (end == UNKNOWN) {
		unsigned short endTest = 0x1234;
		unsigned char* firstByte = (unsigned char*)(&endTest);

		if (*firstByte == 0x34)
			end = LIL;
		else
			end = BIG;
	}

	if (end == BIG)
		return;

	unsigned short i = 0;
	unsigned short j = sz - 1;

	while (i < j) {
		char temp = bytes[i];
		bytes[i] = bytes[j];
		bytes[j] = temp;
		i++;
		j--;
	}
}

short wsa_create() {
	if (wsaData != NULL)
		return 0;

	wsaData = malloc(sizeof(WSADATA));
	if (wsaData == NULL)
		return -1;

	return WSAStartup(MAKEWORD(2, 2), wsaData);
}

unsigned short wsa_destroy() {
	if (wsaData == NULL)
		return 0;

	if (WSACleanup() == SOCKET_ERROR)
		return WSAGetLastError();

	free(wsaData);
	wsaData = NULL;

	return 0;
}
