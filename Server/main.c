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
	struct Server* server = server_create(3490);
	struct Player players[2];

	players[0].x = 0;
	players[0].y = 0;
	players[1].x = 0;
	players[1].y = 0;

	char buf[1024];
	char state[18];
	while (!(GetAsyncKeyState(VK_END) & 0x01)) {
		server_sync(server, buf);

		unsigned char* i = buf;
		while (*i != '\0') {
			int player = (*i & 0b10000000) >> 7;
			int numBytes = *i & 0b01111111;
			int seq = (i[1] << 8) | i[2];

			int dir = i[3] * 45;
			float mag = unpack_float(i + 4);

			players[player].x += (float)cos(dir * PI / 180) * mag;
			players[player].y -= (float)sin(dir * PI / 180) * mag;

			printf("%i bytes from p%i seq %i: %f, %f\n", numBytes, player, seq, players[0].x, players[0].y);
			//printf("p2: %f, %f\n", players[1].x, players[1].y);

			i += numBytes + 1;
		}

		//printf("l");
		//pack_float(players[0].x, state + 2);
		//pack_float(players[0].y, state + 6);
		//pack_float(players[1].x, state + 10);
		//pack_float(players[1].y, state + 14);
		//
		//server_ping(server, state);
	}
}