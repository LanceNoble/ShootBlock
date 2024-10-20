#pragma once

struct Server* server_create(const char* const port, unsigned char max, unsigned char** data, union Bump** bumps);
void server_sync(struct Server* server, unsigned char* numBumpsPtr);
void server_bump(struct Server* server, unsigned char id, unsigned char start, unsigned char size, unsigned char* val);
void server_destroy(struct Server** server);