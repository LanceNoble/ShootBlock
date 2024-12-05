#include <Windows.h>

#include "convert.h"
#include "comms.h"
#include "server.h"

#include <math.h>
#include <stdio.h>

#define PI 3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067

struct Player {
	float x;
	float y;
};

int main() {

	void* server = server_create(3490);
	struct Message state;
	struct Player ps[MAX_PLAYERS];
	ps[0].x = 0;
	ps[0].y = 0;
	ps[1].x = 0;
	ps[1].y = 0;

	state.len = 16;
	while (!(GetAsyncKeyState(VK_END) & 0x01)) {
		struct Host* players = server_sync(server);
		//printf("%p\n", players);
		//printf("p1 sent %i msgs\n", players[0].numMsgs);
		//printf("p2 sent %i msgs ", players[0].numMsgs);
		
		for (int i = 0; i < MAX_PLAYERS; i++) {
			for (int j = 0; j < players[i].numMsgs; j++) {
				//printf("yap\n");

				union PackedFloat pf;
				// We start accessing client messages from the 3rd byte
				// First two bytes are sequence bytes
				// Actual input from the client starts at the 3rd byte
				pf.raw[0] = players[i].msgs[j].buf[3];
				pf.raw[1] = players[i].msgs[j].buf[4];
				pf.raw[2] = players[i].msgs[j].buf[5];
				pf.raw[3] = players[i].msgs[j].buf[6];
				float mag = unpack_float(pf.pack);

				
				float xOff = cos(players[i].msgs[j].buf[2] * 45 * PI / 180);
				float yOff = sin(players[i].msgs[j].buf[2] * 45 * PI / 180);

				ps[i].x += xOff * mag;
				ps[i].y += yOff * mag;

				printf("p%i pos: %f, %f\n", i, ps[i].x, ps[i].y);
				//printf("p%i move dir %i mag %f\n", i, players[i].msgs[j].buf[2], mag);
			}
		}


		struct Message m;
		m.len = 16;
		
		union PackedFloat pfs[MAX_PLAYERS * 2];
		pfs[0].pack = pack_float(ps[0].x);
		pfs[1].pack = pack_float(ps[0].y);
		pfs[2].pack = pack_float(ps[1].x);
		pfs[3].pack = pack_float(ps[1].y);

		m.buf[0]  = pfs[0].raw[0];
		m.buf[1]  = pfs[0].raw[1];
		m.buf[2]  = pfs[0].raw[2];
		m.buf[3]  = pfs[0].raw[3];

		m.buf[4]  = pfs[1].raw[0];
		m.buf[5]  = pfs[1].raw[1];
		m.buf[6]  = pfs[1].raw[2];
		m.buf[7]  = pfs[1].raw[3];

		m.buf[8]  = pfs[2].raw[0];
		m.buf[9]  = pfs[2].raw[1];
		m.buf[10] = pfs[2].raw[2];
		m.buf[11] = pfs[2].raw[3];

		m.buf[12] = pfs[3].raw[0];
		m.buf[13] = pfs[3].raw[1];
		m.buf[14] = pfs[3].raw[2];
		m.buf[15] = pfs[3].raw[3];

		/*
		for (int k = 0; k < MAX_PLAYERS; k++) {
			pfs[k].pack = pack_float(ps[k].x);
			for (int l = 0; l < 8; l++) {
				m.buf[(k * 8 + l)] = 0;
			}
		}
		*/

		server_ping(server, m);
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