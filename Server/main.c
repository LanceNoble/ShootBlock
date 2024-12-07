#include "convert.h"
#include "comms.h"
#include "server.h"

#include <Windows.h>
#include <time.h>
#include <math.h>
#include <stdio.h>

#define PI 3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067

struct Player {
	float x;
	float y;
};

int main() {
	clock_t t = 0;

	struct Server* server = server_create(3490);
	struct Player players[2];
	players[0].x = 0;
	players[0].y = 0;
	players[1].x = 0;
	players[1].y = 0;
	char inputBuf[256];

	while (!(GetAsyncKeyState(VK_END) & 0x01)) {
		server_sync(server, inputBuf);
		
		unsigned char* i = inputBuf;
		while (*i != '\0') {
			int player = (*i & 0b10000000) >> 7;
			int numBytes = *i & 0b01111111;
			int seq = (i[1] << 8) | i[2];
			int dir = i[3] * 45;
			float mag = unpack_float(i + 4);


			//printf("Sequence %i: %i bytes from p%i\n", (i[1] << 8) | i[2], numBytes, (*i & 0b10000000) >> 7);
			printf("p%i move %i at %f\n", player, dir, mag);


			i += numBytes + 1;
		}

		/*
		for (int i = 0; i < MAX_PLAYERS; i++) {
			for (int j = 0; j < players[i].numMsgs; j++) {
				float mag = unpack_float(players[i].msgs[j].buf + 3);

				
				float xOff = cos(players[i].msgs[j].buf[2] * 45 * PI / 180);
				float yOff = sin(players[i].msgs[j].buf[2] * 45 * PI / 180);

				ps[i].x += xOff * mag;
				ps[i].y += yOff * mag;

				//printf("p%i pos: %f, %f\n", i, ps[i].x, ps[i].y);
				//printf("p%i move dir %i mag %f\n", i, players[i].msgs[j].buf[2], mag);
			}
		}

		if ((clock() - t) / CLOCKS_PER_SEC >= 0) {
			//t = clock();
			struct Message m;
			m.len = 16;

			pack_float(ps[0].x, m.buf);
			pack_float(ps[0].y, m.buf + 4);
			pack_float(ps[1].x, m.buf + 8);
			pack_float(ps[1].y, m.buf + 12);

			server_ping(server, m);
		}
		*/

		//printf("looping");
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