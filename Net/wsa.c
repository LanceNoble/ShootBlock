#include "comms.h"

int wsa_create() {

	// if WSAStartup was already called
	if (wsaData != NULL)
		return 0;

	wsaData = malloc(sizeof(WSADATA));

	// if malloc failed to allocate
	if (wsaData == NULL)
		return 1;

	unsigned int status = WSAStartup(MAKEWORD(2, 2), wsaData);
	return status;
}
int wsa_destroy() {
	
	// if WSACleanup was already called
	if (wsaData == NULL)
		return 0;

	int status = WSACleanup();
	if (status == SOCKET_ERROR)
		status = WSAGetLastError();

	free(wsaData);
	wsaData = NULL;

	return status;
}