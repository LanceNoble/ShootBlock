#pragma once
#define MAX_PLAYERS 2

void* server_create(unsigned short port);
struct Host* server_sync(void* server);
short server_ping(struct Server* server);
void server_destroy(struct Server** server);