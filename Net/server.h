#pragma once

void* server_create(unsigned short port);
unsigned short server_sync(void* server);
short server_ping(struct Server* server);
void server_destroy(struct Server** server);