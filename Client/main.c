#include "raylib.h"

#include "convert.h"
#include "comms.h"
#include "client.h"

#include <math.h>
#include <stdio.h>

int main() {
	void* client = client_create("127.0.0.1", 3490);
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(1280, 800, "ShootBlock");

	char buf[1024];
	char input[7];
	int i = 0;
	while (!WindowShouldClose()) {
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
			float mag = (float)sqrt(pow(xOff, 2) + pow(yOff, 2));
			xOff /= mag;
			yOff /= mag;

			float rad = (float)atan(yOff / xOff);
			float deg = (rad * 180) / PI;
			if (xOff < 0) {
				deg += 180;
			}
			else if (yOff < 0) {
				deg += 360;
			}

			float spd = 250.0f;
			input[2] = (int)(deg / 45);
			
			pack_float(spd * GetFrameTime(), input + 3);
			//unpack_float(input + 3);
			//printf("%f\n", unpack_float(input + 3));
			//printf("%i\n", i);
			//i++;
			client_ping(client, input);
		}
		
		unsigned char* state = client_sync(client, buf, &state);
		if (state != NULL) {
			Vector2 p1Pos;
			Vector2 p2Pos;
			Vector2 sz;
			p1Pos.x = unpack_float(state + 1 + 2);
			p1Pos.y = unpack_float(state + 1 + 6);
			p2Pos.x = unpack_float(state + 1 + 10);
			p2Pos.y = unpack_float(state + 1 + 14);
			sz.x = 50;
			sz.y = 50;
			DrawRectangleV(p1Pos, sz, WHITE);
			DrawRectangleV(p2Pos, sz, WHITE);
			//printf("P1 Pos: %f, %f\n", unpack_float(state + 2), unpack_float(state + 6));
			//printf("P2 Pos: %f, %f\n", unpack_float(state + 10), unpack_float(state + 14));

			//printf("%i", *state);
		}
		//printf("looping\n");

		/*
		struct Message* potentialState = client_sync(client);
		if (potentialState != NULL) {
			state = potentialState;
		}
		if (state != NULL) {
			//printf("unpacking");
			Vector2 p1Pos;
			p1Pos.x = unpack_float(state->buf + 2);
			p1Pos.y = -unpack_float(state->buf + 6);
			Vector2 p2Pos;
			p2Pos.x = unpack_float(state->buf + 10);
			p2Pos.y = -unpack_float(state->buf + 14);
			Vector2 sz;
			sz.x = 50;
			sz.y = 50;
			DrawRectangleV(p1Pos, sz, WHITE);
			DrawRectangleV(p2Pos, sz, WHITE);
			//printf("P1 Pos: %f, %f\n", unpack_float(p1x.pack), unpack_float(p1y.pack));
			//printf("P2 Pos: %f, %f\n", unpack_float(p2x.pack), unpack_float(p2y.pack));
		}
		*/
		//printf("looping");
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