#pragma once

struct Client* client_create(const char* const ip, const unsigned short port);
void client_ping(struct Client* client, char* input);
void client_sync(struct Client* client);
void client_destroy(struct Client* client);