#include "raylib.h"

#include "convert.h"
#include "comms.h"
#include "client.h"

#include <math.h>
#include <stdio.h>

int main() {
	
	struct Message* state = NULL;
	void* client = client_create("127.0.0.1", 3490);
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(1280, 800, "ShootBlock");
	
	while (!(WindowShouldClose())) {
		BeginDrawing();
		ClearBackground(BLACK);

		float xOff = 0;
		float yOff = 0;
		if (IsKeyDown(KEY_W)) {
			yOff += 1;
		}
		if (IsKeyDown(KEY_A)) {
			xOff -= 1;
		}
		if (IsKeyDown(KEY_S)) {
			yOff -= 1;
		}
		if (IsKeyDown(KEY_D)) {
			xOff += 1;
		}

		if (xOff != 0 || yOff != 0) {
			//printf("moving\n");
			float mag = sqrt(pow(xOff, 2) + pow(yOff, 2));
			xOff /= mag;
			yOff /= mag;

			float rad = atan(yOff / xOff);
			int deg = (rad * 180) / PI;
			if (xOff < 0) {
				deg += 180;
			}
			else if (yOff < 0) {
				deg += 360;
			}

			float spd = 250.0f;
			struct Message ping;
			ping.len = 5;
			ping.buf[0] = deg / 45;
			union PackedFloat pf;
			pf.pack = pack_float(spd * GetFrameTime());
			ping.buf[1] = pf.raw[0];
			ping.buf[2] = pf.raw[1];
			ping.buf[3] = pf.raw[2];
			ping.buf[4] = pf.raw[3];
			client_ping(client, ping);
		}
		struct Message* potentialState = client_sync(client);
		if (potentialState != NULL) {
			state = potentialState;
		}
		if (state != NULL) {
			/*
			for (int i = 0; i < 2; i++) {

			}
			*/
			union PackedFloat p1x;
			p1x.raw[0] = state->buf[2];
			p1x.raw[1] = state->buf[3];
			p1x.raw[2] = state->buf[4];
			p1x.raw[3] = state->buf[5];
			union PackedFloat p1y;
			p1y.raw[0] = state->buf[6];
			p1y.raw[1] = state->buf[7];
			p1y.raw[2] = state->buf[8];
			p1y.raw[3] = state->buf[9];
			union PackedFloat p2x;
			p2x.raw[0] = state->buf[10];
			p2x.raw[1] = state->buf[11];
			p2x.raw[2] = state->buf[12];
			p2x.raw[3] = state->buf[13];
			union PackedFloat p2y;
			p2y.raw[0] = state->buf[14];
			p2y.raw[1] = state->buf[15];
			p2y.raw[2] = state->buf[16];
			p2y.raw[3] = state->buf[17];

			printf("P1 Pos: %f, %f\n", unpack_float(p1x.pack), unpack_float(p1y.pack));
			printf("P2 Pos: %f, %f\n", unpack_float(p2x.pack), unpack_float(p2y.pack));
		}

		EndDrawing();
	}

	if (client == NULL) {
		printf("No feedback from server. Closing...\n");
	}
	if (client != NULL) {
		printf("You Quit.\n");
		client_destroy(client);
	}
}