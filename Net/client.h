#pragma once

struct Client* client_create(const char* const ip, const char* const port, unsigned char** data, union Meta* info);
void client_bump(struct Client* client, unsigned char id, unsigned char start, unsigned char size, unsigned char* val);
void client_sync(struct Client* client);
void client_destroy(struct Client** client);