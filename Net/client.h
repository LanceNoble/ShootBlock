#pragma once

struct Client* client_create(const char* const ip, const unsigned short port);
unsigned short client_ping(struct Client* client, struct Message msg);
struct Message* client_sync(struct Client* client);
void client_destroy(struct Client* client);