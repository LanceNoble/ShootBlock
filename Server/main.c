#include <Windows.h>
#include <stdio.h>

#include "wsa.h"
#include "comms.h"
#include "server.h"

int main() {
	wsa_create();

	unsigned char* data;
	union Bump* bumps;
	struct Server* server = server_create("3490", 4, &data, &bumps);

	while (!(GetAsyncKeyState(VK_END) & 0x01)) {
		unsigned char numBumps;
		server_sync(server, &numBumps);
		
		/*
		for (int i = 0; i < SERVER_MAX; i++) {
			if (bumps->start == CON_BYTE) {
				for (int i = 0; i < SERVER_MAX; i++) {
					printf("Player %i is ", i);
					if (data[i * PLAYER_SIZE] == CON_BYTE_ON)
						printf("Online");
					else if (data[i * PLAYER_SIZE] == CON_BYTE_OFF)
						printf("Offline");
					printf("\n");
				}
				break;
			}
		}
		*/

	}

	server_destroy(&server);
	wsa_destroy();
}