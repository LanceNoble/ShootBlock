#pragma once

struct Client* client_create(const char* const ip, const char* const port, unsigned char** data);
unsigned short client_sync(struct Client* client);
unsigned short client_bump(struct Client* client, unsigned char start, unsigned char size, unsigned char* val);
void client_destroy(struct Client** client);