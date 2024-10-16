#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#include "mem.h"

#define MAX_PLAYERS 4
#define PORT "3490"

int wsa_create();
void flip(char* bytes, const unsigned short sz);
int wsa_destroy();

struct MSG* MSG_create();
void MSG_set(struct MSG* m, char* buf, const unsigned short sz);
int MSG_send(struct MSG* m, SOCKET* s);
int MSG_recv(struct MSG* m, SOCKET* s);
void MSG_destroy(struct MSG** m);

union Msg* msg_create();
void msg_format(union Msg* m, unsigned int type, unsigned int id, unsigned int xPos, unsigned int yPos);
void msg_flip(union Msg* m);
int msg_send(union Msg* m, SOCKET* s);
int msg_fetch(union Msg* m, SOCKET* s, unsigned int* type, unsigned int* id, unsigned int* xPos, unsigned int* yPos);
void msg_destroy(union Msg** m);

unsigned int float_pack(float den);
float float_unpack(unsigned int bin);