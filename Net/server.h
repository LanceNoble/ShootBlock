#pragma once

struct Server* server_create(unsigned char max, unsigned char size, const char* const port);
void server_relay(struct Server* server);
void server_destroy(struct Server** server);