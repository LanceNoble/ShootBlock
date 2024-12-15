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
	SOCKET tcp;

	unsigned short seq;
	unsigned short ack;

	struct sockaddr_in name;
	clock_t time;
	unsigned short remSeq;

	struct Input* firstIn;
	struct Input* lastIn;
};

struct Client* client_create() {
	struct Client* client = malloc(sizeof(struct Client));

	if (client == NULL || WSAStartup(MAKEWORD(2, 2), &client->wsa) != 0) {
		client_destroy(client);
		return NULL;
	}

	client->seq = 0;
	client->ack = 0;
	client->name.sin_family = AF_INET;
	client->name.sin_addr.S_un.S_addr = 0;
	
	client->time = clock();
	client->remSeq = 0;
	client->firstIn = NULL;
	client->lastIn = NULL;

	u_long blockMode = 1;
	client->tcp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	client->udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	if (client->udp == INVALID_SOCKET || 
		client->tcp == INVALID_SOCKET ||
		ioctlsocket(client->udp, FIONBIO, &blockMode) == SOCKET_ERROR) {

		client_destroy(client);
		return NULL;
	}

	return client;
}

int client_join(struct Client* client, const unsigned char* ip, const unsigned short port) {
	int len = 0;
	while (*ip != '\0') {
		len++;
		ip++;
	}
	ip--;

	int digit = 0;
	int val = 0;
	int bit = 24;

	while (len > 0) {
		if (*ip == '.') {
			client->name.sin_addr.S_un.S_addr |= (val << bit);
			digit = 0;
			val = 0;
			bit -= 8;
		}
		else {
			unsigned char place = 1;
			for (unsigned char i = 0; i < digit; i++) {
				place *= 10;
			}
			val += (*ip - 48) * place;
			digit++;
		}

		len--;
		ip--;
	}

	client->name.sin_addr.S_un.S_addr |= val;
	client->name.sin_port = htons(port);

	unsigned char buf[1];
	int res;

	res = connect(client->tcp, (struct sockaddr*)&client->name, sizeof(struct sockaddr));
	res = recv(client->tcp, buf, 1, 0);

	if (res == SOCKET_ERROR || res == 0) {
		client->name.sin_addr.S_un.S_addr = 0;
		return res;
	}

	closesocket(client->tcp);

	return buf[0];
}

void client_ping(struct Client* client, unsigned char* buf) {
	client->seq++;

	buf[0] = 0;
	buf[1] = 0;
	buf[0] = (client->seq & (0xff << 8)) >> 8;
	buf[1] = client->seq & 0xff;

	struct Input* link = malloc(sizeof(struct Input));
	if (link == NULL) {
		return;
	}

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

	sendto(client->udp, buf, 7, 0, (struct sockaddr*)&client->name, sizeof(struct sockaddr));
}

unsigned char* client_sync(struct Client* client, unsigned char* buf) {
	unsigned char* state = NULL;
	unsigned char* iBuf = buf;
	unsigned char* meta;

	do {
		meta = iBuf++;
		*meta = recvfrom(client->udp, iBuf, 256, 0, NULL, NULL);
		iBuf += *meta;
	} while (*meta != (unsigned char)SOCKET_ERROR);

	*meta = '\0';
	iBuf = buf;
	union Response res;
	unsigned short seq;

	while (*iBuf != '\0') {
		res.raw[0] = iBuf[1];
		res.raw[1] = iBuf[2];
		res.raw[2] = iBuf[3];
		res.raw[3] = iBuf[4];

		seq = (iBuf[1] << 8) | iBuf[2];

		if (*iBuf == sizeof(union Response) && res.ack > client->ack) {
			client->ack = res.ack;

			int numAcks = 0;
			unsigned short acks[17];

			for (int i = 0; i < 17; i++) {
				if (i == 0) {
					acks[numAcks++] = res.ack;
				}
				else if (res.bit & (1 << (i - 1))) {
					acks[numAcks++] = (unsigned short)res.ack + i;
				}
			}

			for (int i = 0; i < numAcks; i++) {
				//printf("acknowledging ack %i\n", acks[i]);
				while (client->firstIn != NULL && client->firstIn->seq <= acks[i]) {
					struct Input* del = client->firstIn;
					client->firstIn = client->firstIn->next;

					if (client->firstIn == NULL) {
						client->lastIn = NULL;
					}

					if (del->seq < acks[i]) {
						//printf("resending sequence %i\n", del->seq);
						client_ping(client, del->val);
					}

					free(del);
				}
			}
		}
		else if (*iBuf > sizeof(union Response) && (seq > client->remSeq || seq < client->remSeq && client->remSeq - seq > 0xffff / 2)) {
			state = iBuf;
			client->remSeq = seq;
			client->time = clock();
		}

		iBuf += *iBuf + 1;
	}

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

void client_leave(struct Client* client) {
	client->name.sin_addr.S_un.S_addr = 0;
	unsigned char buf[1];
	int res;
	res = connect(client->tcp, (struct sockaddr*)&client->name, sizeof(struct sockaddr));
	res = recv(client->tcp, buf, 1, 0);
	client->name.sin_addr.S_un.S_addr = 0;
	closesocket(client->tcp);
}

static void input_destroy(struct Input* in) {
	if (in == NULL) {
		return;
	}
	input_destroy(in->next);
	free(in);
}

void client_destroy(struct Client* client) {
	WSACleanup();
	closesocket(client->tcp);
	closesocket(client->udp);
	input_destroy(client->firstIn);
	free(client);
}