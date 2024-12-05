#include "comms.h"

#include <winsock2.h>

static WSADATA* wsa;

// Initialize WSA
short wsa_create() {
	if (wsa != NULL) {
		return 0;
	}
	wsa = malloc(sizeof(WSADATA));
	if (wsa == NULL) {
		return -1;
	}
	return WSAStartup(MAKEWORD(2, 2), wsa);
}

// Clean up WSA initialization resources
unsigned short wsa_destroy() {
	if (wsa == NULL) {
		return 0;
	}
	if (WSACleanup() == SOCKET_ERROR) {
		return WSAGetLastError();
	}
	free(wsa);
	wsa = NULL;
	return 0;
}
