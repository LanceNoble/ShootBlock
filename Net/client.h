#pragma once

void* client_create(const char* const ip, const unsigned short port);
unsigned short client_ping(void* client, struct Message msg);
struct Message client_sync(void** client);
void client_destroy(void* client);