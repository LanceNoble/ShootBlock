#pragma once

//short server_create(unsigned short port, struct Server** server);
short server_sync(struct Server* server);
short server_ping(struct Server* server);
void server_destroy(struct Server** server);