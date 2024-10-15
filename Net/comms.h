#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#define MAX_PLAYERS 4
#define PORT "3490"

extern WSADATA* wsaDataFUCK;
int wsa_create();
int wsa_destroy();

union Message* message_create();
void message_format(union Message* m, unsigned int type, unsigned int id, unsigned int xPos, unsigned int yPos);
void message_flip(union Message* m);
int message_send(union Message* m, SOCKET* s);
int message_fetch(union Message* m, SOCKET* s, unsigned int* type, unsigned int* id, unsigned int* xPos, unsigned int* yPos);
void message_destroy(union Message** m);

unsigned int float_pack(float den);
float float_unpack(unsigned int bin);