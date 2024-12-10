#include "client.h"

#include <winsock2.h>
#include <time.h>
#include <stdio.h>

#include "comms.h"

struct Input {
	unsigned short seq;
	clock_t time;
	unsigned char val[7];

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

void client_ping(struct Client* client, unsigned char* buf) {
	buf[0] = 0;
	buf[1] = 0;

	buf[0] = (client->seq & (0xff << 8)) >> 8;
	buf[1] = client->seq & 0xff;

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

		for (int i = 0; i < 7; i++) {
			link->val[i] = buf[i];
		}

		sendto(client->udp, buf, 7, 0, (struct sockaddr*)&client->addr, sizeof(struct sockaddr));
		client->seq++;
	}
}

unsigned char* client_sync(struct Client* client, unsigned char* buf) {
	unsigned char* state = NULL;
	char* i = buf;
	unsigned char* meta;
	int numMsgs = 0;

	do {
		meta = i++;
		*meta = recvfrom(client->udp, i, 32, 0, NULL, NULL);
		i += *meta;

		if (*meta == sizeof(union Response)) {
			union Response res;
			res.raw[0] = meta[1];
			res.raw[1] = meta[2];
			res.raw[2] = meta[3];
			res.raw[3] = meta[4];

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
		else if (*meta != (unsigned char)SOCKET_ERROR) {
			unsigned short seq = (meta[1] << 8) | meta[2];
			if (seq > client->remSeq || seq < client->remSeq && client->remSeq - seq > 0xffff / 2) {
				state = meta;
				client->remSeq = seq;
				client->time = clock();
				numMsgs++;
			}
			else {
				i = meta;
			}
		}
	} while (*meta != (unsigned char)SOCKET_ERROR && numMsgs < 32);
	
	if ((clock() - client->time) / CLOCKS_PER_SEC >= TIMEOUT_HOST) {
		client_destroy(client);
		return NULL;
	}

	if (client->firstIn != NULL && (clock() - client->firstIn->time) / CLOCKS_PER_SEC >= TIMEOUT_PACKET) {
		struct Input* del = client->firstIn;
		client->firstIn = client->firstIn->next;
		client_ping(client, del->val);
		free(del);
	}
	
	return state;
}

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