#pragma once

struct Server* server_create(unsigned short port);
void server_sync(struct Server* server, char* inputBuf);
void server_ping(struct Server* server, struct Message state);
void server_destroy(struct Server* server);