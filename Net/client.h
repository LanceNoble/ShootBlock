#pragma once

struct Client* client_create(const char* const ip, const unsigned short port);
void client_ping(struct Client* client, unsigned char* buf);
void client_sync(struct Client* client, char* buf, char** state);
void client_destroy(struct Client* client);