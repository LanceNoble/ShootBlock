#include "comms.h"

#define LIL_END 0
#define BIG_END 1

static WSADATA* wsaData;
static unsigned char end;

int wsa_create() {
	if (wsaData != NULL)
		return 0;

	wsaData = malloc(sizeof(WSADATA));
	if (wsaData == NULL)
		return 1;

	unsigned short status = WSAStartup(MAKEWORD(2, 2), wsaData);
	if (status != 0)
		return status;

	unsigned short endTest = 0x1234;
	unsigned char* firstByte = (unsigned char*)(&endTest);
	if (*firstByte == 0x34) {
		end = LIL_END;
		return 0;
	}

	end = BIG_END;
	return 0;
}

void flip(char* bytes, const unsigned short sz) {
	if (end == BIG_END)
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

int wsa_destroy() {
	if (wsaData == NULL)
		return 0;
	
	if (WSACleanup() == SOCKET_ERROR)
		return WSAGetLastError();

	free(wsaData);
	wsaData = NULL;

	return 0;
}