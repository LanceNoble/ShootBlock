#pragma once

struct Server* server_create(unsigned short port);
void server_sync(struct Server* server);
void server_ping(struct Server* server);
void server_destroy(struct Server** server);