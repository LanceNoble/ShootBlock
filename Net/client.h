#pragma once

short client_create(unsigned char b0, unsigned char b1, unsigned char b2, unsigned char b3, unsigned short port, struct Client** client);
short client_ping(struct Client* client);
short client_sync(struct Client* client);
void client_destroy(struct Client** client);