#pragma once

struct Client* client_create();
void client_ping(struct Client* client, unsigned char* buf);
unsigned char* client_sync(struct Client* client, unsigned char* buf);
void client_destroy(struct Client* client);