#pragma once

void* server_create(unsigned short port);
struct Host* server_sync(void* server);
void server_ping(void* server, struct Message state);
void server_destroy(struct Server** server);