#include "comms.h"

int wsa_create() {

	// if WSAStartup was already called
	if (wsaDataFUCK != NULL)
		return 0;

	wsaDataFUCK = malloc(sizeof(WSADATA));

	// if malloc failed to allocate
	if (wsaDataFUCK == NULL)
		return 1;

	unsigned int status = WSAStartup(MAKEWORD(2, 2), wsaDataFUCK);
	return status;
}
int wsa_destroy() {
	
	// if WSACleanup was already called
	if (wsaDataFUCK == NULL)
		return 0;

	int status = WSACleanup();
	if (status == SOCKET_ERROR)
		status = WSAGetLastError();

	free(wsaDataFUCK);
	wsaDataFUCK = NULL;

	return status;
}