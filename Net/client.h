#pragma once

struct Client* client_create(unsigned long byte0, unsigned long byte1, unsigned long byte2, unsigned long byte3, unsigned short port);
void client_ping(struct Client* client);
void client_sync(struct Client* client);
void client_destroy(struct Client** client);