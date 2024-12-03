#include <Windows.h>

#include "convert.h"
#include "comms.h"
#include "server.h"

#include <stdio.h>

int main() {
	short res = wsa_create();
	if (res != 0) {
		printf("WSA Init Fail\n");
		return res;
	}

	void* server = server_create(3490);
	while (!(GetAsyncKeyState(VK_END) & 0x01)) {
		struct Host* players = server_sync(server);
		//printf("%p\n", players);
		//printf("FUCK");
		//printf("p1 sent %i msgs\n", players[0].numMsgs);
		//printf("p2 sent %i msgs ", players[0].numMsgs);
		
		for (int i = 0; i < MAX_PLAYERS; i++) {
			for (int j = 0; j < players[i].numMsgs; j++) {
				//printf("yap\n");

				union PackedFloat pf;
				pf.raw[0] = players[i].msgs[j].buf[1];
				pf.raw[1] = players[i].msgs[j].buf[2];
				pf.raw[2] = players[i].msgs[j].buf[3];
				pf.raw[3] = players[i].msgs[j].buf[4];
				float mag = 1.0f;//unpack_float(pf.pack);
				printf("p%i move dir %i mag %f\n", i, players[i].msgs[j].buf[2], mag);
			}
		}
				
	}

	/*
	if (res != 0) {
		printf("Server Died\n");
		server_destroy(&server);
		wsa_destroy();
		return res;
	}

	while (!(GetAsyncKeyState(VK_END) & 0x01)) {
		server_sync(server);
		server_ping(server);
	}

	server_destroy(&server);
	wsa_destroy();
	*/
	
}