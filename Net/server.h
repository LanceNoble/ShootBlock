#pragma once
#define MAX_PLAYERS 2

void* server_create(unsigned short port);
unsigned short server_sync(void* server, struct Host* players);
short server_ping(struct Server* server);
void server_destroy(struct Server** server);