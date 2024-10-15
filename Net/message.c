#include "comms.h"

// How the client and server will MOSTLY format their messages when talking to each other
// The netcode is written in a way where the client and server won't always use this format
// For example:
//	1. a client's connect() also means Player Join
//	2. a client's or server's closesocket() also means Player Leave
// 
// The union lets us represent the message in 3 forms:
// 1. Struct Form makes it easier to format the message
// 2. Int Form makes it easier to use hton and ntoh functions
// 3. Char Array Form makes it easier to send and recv
//
// Not all message types will utilize all bit fields
union Message {
	struct {
		// 1 - Player Join (Server to Player only)
		// 2 - Player Leave (Server to Player only)
		// 3 - Player Move (Two way)
		unsigned int type : 2;

		// ID of player who sent the message
		unsigned int id : 3;

		// Position of player who sent the message
		unsigned int xPos;
		unsigned int yPos;
	};
	unsigned int whole[3];
	unsigned char raw[12];
};
union Message* message_create() {
	union Message* m = malloc(sizeof(union Message));
	if (m == NULL)
		return NULL;
	return m;
}
void message_format(union Message* m, unsigned int type, unsigned int id, unsigned int xPos, unsigned int yPos) {
	m->type = type;
	m->id = id;
	m->xPos = xPos;
	m->yPos = yPos;
}
void message_flip(union Message* m) {
	m->whole[0] = htonl(m->whole[0]);
	m->whole[1] = htonl(m->whole[1]);
	m->whole[2] = htonl(m->whole[2]);
}
int message_send(union Message* m, SOCKET* s) {
	int numBytes = send(*s, m->raw, sizeof(*m), 0);
	if (numBytes == SOCKET_ERROR)
		return WSAGetLastError();
	return numBytes;
}
int message_fetch(union Message* m, SOCKET* s, unsigned int* type, unsigned int* id, unsigned int* xPos, unsigned int* yPos) {
	message_format(m, 0, 0, 0, 0);

	int numBytes = recv(*s, m->raw, sizeof(*m), 0);
	if (numBytes == SOCKET_ERROR)
		return WSAGetLastError();

	m->whole[0] = ntohl(m->whole[0]);
	m->whole[1] = ntohl(m->whole[1]);
	m->whole[2] = ntohl(m->whole[2]);

	if (type != NULL)
		*type = m->type;
	if (id != NULL)
		*id = m->id;
	if (xPos != NULL)
		*xPos = m->xPos;
	if (yPos != NULL)
		*yPos = m->yPos;

	return numBytes;
}
void message_destroy(union Message** m) {
	free(*m);
	*m = NULL;
}