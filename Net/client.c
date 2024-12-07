#include "client.h"

#include <winsock2.h>
#include <time.h>
#include <stdio.h>

#include "comms.h"

struct Input {
	unsigned short seq;
	long time;
	char val[5];

	struct Input* next;
};

struct Client {
	WSADATA wsa;
	SOCKET udp;
	unsigned short seq;
	unsigned short ack;

	struct sockaddr_in addr;
	clock_t time;
	unsigned short remSeq;

	struct Input* firstIn;
	struct Input* lastIn;
};

struct Client* client_create(const char* const ip, const unsigned short port) {
	struct Client* client = malloc(sizeof(struct Client));
	if (client == NULL) {
		printf("Client Creation Fail: No Memory\n");
		return NULL;
	}

	if (WSAStartup(MAKEWORD(2, 2), &client->wsa) != 0) {
		printf("Client Creation Fail: WSAStartup Fail\n");
		return NULL;
	}

	// Server only accepts sequences greater than the most recent sequence it received
	// So 
	client->seq = 1;
	client->ack = 0;

	client->addr.sin_family = AF_INET;
	client->addr.sin_addr.S_un.S_addr = 0;
	client->addr.sin_port;
	const unsigned char* letter = ip;
	signed char len = 0;
	while (*letter != '\0') {
		len++;
		letter++;
	}
	letter--;
	unsigned char digit = 0;
	unsigned char val = 0;
	unsigned char bit = 24;
	while (len > 0) {
		if (*letter == '.') {
			client->addr.sin_addr.S_un.S_addr |= (val << bit);
			digit = 0;
			val = 0;
			bit -= 8;
		}
		else {
			unsigned char place = 1;
			for (unsigned char i = 0; i < digit; i++) {
				place *= 10;
			}
			val += (*letter - 48) * place;
			digit++;
		}
		len--;
		letter--;
	}
	client->addr.sin_addr.S_un.S_addr |= val;
	client->addr.sin_port = htons(port);
	client->time = clock();
	client->remSeq = 0;
	client->firstIn = NULL;
	client->lastIn = NULL;

	client->udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (client->udp == INVALID_SOCKET) {
		printf("Client Creation Fail: Invalid Socket (%u)\n", WSAGetLastError());
		client_destroy(client);
		return NULL;
	}

	unsigned long mode = 1;
	if (ioctlsocket(client->udp, FIONBIO, &mode) == SOCKET_ERROR) {
		printf("Client Creation Fail: Can't No-Block (%u)\n", WSAGetLastError());
		client_destroy(client);
		return NULL;
	}

	struct sockaddr_in bindAddr;
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	bindAddr.sin_port = 0;
	if (bind(client->udp, (struct sockaddr*)&bindAddr, (unsigned long)sizeof(struct sockaddr)) == SOCKET_ERROR) {
		printf("Client Creation Fail: Can't Bind (%u)\n", WSAGetLastError());
		client_destroy(client);
		return NULL;
	}

	return client;
}

void client_ping(struct Client* client, char* input) {
	unsigned char sendBuf[7];
	sendBuf[0] = (client->seq & (0xff << 8)) >> 8;
	sendBuf[1] = client->seq & 0xff;

	struct Input* link = malloc(sizeof(struct Input));
	if (link != NULL) {
		if (client->firstIn == NULL) {
			client->firstIn = link;
			client->lastIn = link;
		}
		else {
			client->lastIn->next = link;
			client->lastIn = client->lastIn->next;
		}

		link->seq = client->seq;
		link->time = clock();
		link->next = NULL;

		for (int i = 2, j = 0; i < 7; i++, j++) {
			link->val[j] = input[j];
			sendBuf[i] = link->val[j];
		}

		//printf("Sending Sequence %u\n", (sendBuf[0] << 8) | sendBuf[1]);
		sendto(client->udp, sendBuf, 7, 0, (struct sockaddr*)&client->addr, sizeof(struct sockaddr));
		//printf("%i\n", WSAGetLastError());
		client->seq++;
	}
}
/*
void client_sync(struct Client* client, char* state) {
	struct sockaddr_in from;
	int fromLen = sizeof(struct sockaddr);
	
	while (1) {

	}

	do {
		client->server.msgs[client->server.numMsgs].len = recvfrom(client->udp, client->server.msgs[client->server.numMsgs].buf, 32, 0, (struct sockaddr*)&from, &fromLen);
		if (client->server.msgs[client->server.numMsgs].len != SOCKET_ERROR) {
			++client->server.numMsgs;
		}
	} while (client->server.msgs[client->server.numMsgs].len != SOCKET_ERROR && client->server.numMsgs < 32);

	for (int i = 0; i < client->server.numMsgs; i++) {
		if (client->server.msgs[i].len == sizeof(union Response)) {
			union Response res;
			res.raw[0] = client->server.msgs[i].buf[0];
			res.raw[1] = client->server.msgs[i].buf[1];
			res.raw[2] = client->server.msgs[i].buf[2];
			res.raw[3] = client->server.msgs[i].buf[3];

			if (res.ack > client->ack) {
				for (int i = 0; i < 17; i++) {
					int ack = -1;
					if (i == 0) {
						ack = res.ack;
					}
					else if (res.bit & (1 << (i - 1))) {
						ack = (unsigned short)res.ack + i;
					}
					if (ack != -1) {
						client->ack = ack;
						while (client->firstIn != NULL && client->firstIn->seq <= ack) {
							struct Input* del = client->firstIn;
							client->firstIn = client->firstIn->next;
							if (del->seq < ack) {
								client_ping(client, del->val);
							}
							free(del);
						}
					}
				}
			}
		}
		else if (client->server.msgs[i].len > sizeof(union Response)) {
			unsigned short seq = (client->server.msgs[i].buf[0] << 8) | client->server.msgs[i].buf[1];
			if (seq > client->server.seq) {
				state = &(client->server.msgs[i]);
				client->server.seq = seq;
			}
		}
		client->server.time = clock();
	}

	if ((clock() - time) / CLOCKS_PER_SEC >= TIMEOUT_HOST) {
		client_destroy(client);
		return state;
	}

	if (client->firstIn != NULL && (clock() - client->firstIn->time) / CLOCKS_PER_SEC >= TIMEOUT_PACKET) {
		struct Input* del = client->firstIn;
		client->firstIn = client->firstIn->next;
		client_ping(client, del->val);
		free(del);
	}
}
*/

static void input_destroy(struct Input* in) {
	if (in == NULL) {
		return;
	}
	input_destroy(in->next);
	free(in);
}

void client_destroy(struct Client* client) {
	closesocket(client->udp);
	input_destroy(client->firstIn);
	free(client);
}