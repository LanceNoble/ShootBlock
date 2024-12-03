#include "client.h"

#include <winsock2.h>
#include <time.h>
#include <stdio.h>

#include "comms.h"

struct Input {
	unsigned short seq;
	long time;
	struct Message val;

	struct Input* next;
};

struct Client {
	unsigned long long udp;
	unsigned short seq;
	unsigned short ack;

	struct Host server;
	struct Input* firstIn;
	struct Input* lastIn;
};

void* client_create(const char* const ip, const unsigned short port) {
	struct Client* client = malloc(sizeof(struct Client));
	if (client == NULL) {
		printf("Client Creation Fail: No Memory\n");
		return NULL;
	}

	client->server.numMsgs = 0;
	client->server.msgs = malloc(sizeof(struct Message) * 255);
	if (client->server.msgs == NULL) {
		printf("Client Creation Fail: No Memory\n");
		client_destroy(client);
		return NULL;
	}

	// Server only accepts sequences greater than the most recent sequence it received
	// So 
	client->seq = 1;
	client->ack = 0;
	client->server.ip = 0;
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
			client->server.ip |= (val << bit);
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
	client->server.ip |= val;
	client->server.port = htons(port);
	client->server.time = clock();
	client->server.seq = 0;
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

unsigned short client_ping(void* client, struct Message msg) {
	short res = 0;
	struct Client* cast = client;

	unsigned char sendLen = 0;
	unsigned char sendBuf[0xff];
	sendBuf[0] = (cast->seq & (0xff << 8)) >> 8;
	sendBuf[1] = cast->seq & 0xff;

	struct Input* link = malloc(sizeof(struct Input));
	if (link != NULL) {
		if (cast->firstIn == NULL) {
			cast->firstIn = link;
			cast->lastIn = link;
		}
		else {
			cast->lastIn->next = link;
			cast->lastIn = cast->lastIn->next;
		}

		link->seq = cast->seq;
		link->time = clock();
		link->next = NULL;
		link->val.len = msg.len;

		for (int i = 2, j = 0; i < link->val.len + 2; i++, j++) {
			link->val.buf[j] = msg.buf[j];
			sendBuf[i] = link->val.buf[j];
		}

		printf("Sending Sequence %u\n", (sendBuf[0] << 8) | sendBuf[1]);
		sendLen = link->val.len + 2;
		flip(sendBuf, sendLen);
		
		struct sockaddr_in to;
		to.sin_family = AF_INET;
		to.sin_addr.S_un.S_addr = cast->server.ip;
		to.sin_port = cast->server.port;


		res = sendto(cast->udp, sendBuf, sendLen, 0, (struct sockaddr*)&to, sizeof(struct sockaddr));
		cast->seq++;
	}

	if (res == SOCKET_ERROR) {
		return WSAGetLastError();
	}
	return res;
}

struct Message* client_sync(void* client) {
	struct Message* state = NULL;
	//state.len = 0;
	struct Client* cast = client;

	struct sockaddr_in from;
	int fromLen = sizeof(struct sockaddr);
	cast->server.numMsgs = 0;
	
	do {
		cast->server.msgs[cast->server.numMsgs].len = recvfrom(cast->udp, cast->server.msgs[cast->server.numMsgs].buf, 16, 0, (struct sockaddr*)&from, &fromLen);
		if (cast->server.msgs[cast->server.numMsgs].len != SOCKET_ERROR) {
			++cast->server.numMsgs;
		}
	} while (cast->server.msgs[cast->server.numMsgs].len != SOCKET_ERROR && cast->server.numMsgs < 255);

	for (int i = 0; i < cast->server.numMsgs; i++) {
		flip(cast->server.msgs[i].buf, cast->server.msgs[i].len);
		if (cast->server.msgs[i].len == sizeof(union Response)) {
			union Response res;
			res.raw[0] = cast->server.msgs[i].buf[0];
			res.raw[1] = cast->server.msgs[i].buf[1];
			res.raw[2] = cast->server.msgs[i].buf[2];
			res.raw[3] = cast->server.msgs[i].buf[3];

			if (res.ack > cast->ack) {
				for (int i = 0; i < 17; i++) {
					int ack = -1;
					if (i == 0) {
						ack = res.ack;
					}
					else if (res.bit & (1 << (i - 1))) {
						ack = (unsigned short)res.ack + i;
					}
					if (ack != -1) {
						cast->ack = ack;
						while (cast->firstIn != NULL && cast->firstIn->seq <= ack) {
							struct Input* del = cast->firstIn;
							cast->firstIn = cast->firstIn->next;
							if (del->seq < ack) {
								client_ping(client, del->val);
							}
							/*
							else {
								printf("Sequence %i has been acknowledged\n", del->seq);
							}
							*/
							free(del);
						}
					}
				}
			}
		}
		else if (cast->server.msgs[i].len > sizeof(union Response)) {
			unsigned short seq = (cast->server.msgs[i].buf[0] << 8) | cast->server.msgs[i].buf[1];
			if (seq > cast->server.seq) {
				state = &(cast->server.msgs[i]);
				/*
				state = cast->server.msgs[i];
				state.len = cast->server.msgs[i].len - 2;
				for (unsigned char j = 0, k = 2; j < state.len; j++, k++) {
					state.buf[j] = cast->server.msgs[i].buf[k];
				}
				*/
				cast->server.seq = seq;
			}
		}
		cast->server.time = clock();
	}

	if ((clock() - cast->server.time) / CLOCKS_PER_SEC >= TIMEOUT_HOST) {
		client_destroy(client);
		//*client = NULL;
		return state;
	}

	if (cast->firstIn != NULL && (clock() - cast->firstIn->time) / 1000 >= TIMEOUT_PACKET) {
		//printf("Sequence %i not acknowledged. Resending...\n", cast->firstIn->seq);
		struct Input* del = cast->firstIn;
		cast->firstIn = cast->firstIn->next;
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

void client_destroy(void* client) {
	struct Client* cast = client;
	closesocket(cast->udp);
	input_destroy(cast->firstIn);
	cast->lastIn = NULL;
	free(client);
}