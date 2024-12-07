#pragma once

// Server houses up to 2 players
struct Server* server_create(const unsigned short port);

// Get punctual player messages
// Overwrite buf arg with message data:
//	1st bit of 1st byte of every new message represents which player sent it
//	Remaining 7 bits represent the message's length
//	Next 2 bytes represent the message's sequence
//  Next message length - 2 bytes represents the actual content of the message
// Segfault if there's more data than the buffer size
// Only accept messages 8 bytes or less
// End of data is denoted by NULL-terminating character
void server_sync(struct Server* server, unsigned char* buf);

void server_ping(struct Server* server, struct Message state);
void server_destroy(struct Server* server);