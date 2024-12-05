#pragma once

struct Server* server_create(unsigned short port);
struct Host* server_sync(struct Server* server);
void server_ping(struct Server* server, struct Message state);
void server_destroy(struct Server* server);