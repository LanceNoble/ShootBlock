#include "raylib.h"

#include "convert.h"
#include "comms.h"
#include "client.h"

#include "state.h"
#include "start.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

void main() {
	//start_init();

	//state_change(start_update, start_draw);

	struct Client* client = client_create();
	unsigned char* recvBuf = malloc(sizeof(unsigned char) * 0xffff);
	unsigned char sendBuf[7];
	
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(1280, 800, "ShootBlock");

	Vector2 p1Pos;
	Vector2 p2Pos;
	Vector2 sz;
	
	p1Pos.x = 0;
	p1Pos.y = 0;
	p2Pos.x = 0;
	p2Pos.y = 0;

	sz.x = 50;
	sz.y = 50;

	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(BLACK);

		//state_invoke();
		/*
		float x = 0;
		float y = 0;

		if (IsKeyDown(KEY_W)) {
			y += 1;
		}
		if (IsKeyDown(KEY_A)) {
			x -= 1;
		}
		if (IsKeyDown(KEY_S)) {
			y -= 1;
		}
		if (IsKeyDown(KEY_D)) {
			x += 1;
		}

		if (x != 0 || y != 0) {
			float mag = (float)sqrt(x * x + y * y);
			x /= mag;
			y /= mag;

			float rad = (float)atan(y / x);
			float deg = (rad * 180) / PI;
			
			if (x < 0) {
				deg += 180;
			}
			else if (y < 0) {
				deg += 360;
			}

			sendBuf[2] = (unsigned char)(deg / 45);
			pack_float(500.0f * GetFrameTime(), sendBuf + 3);
			
			p1Pos.x += (float)cos(sendBuf[2] * 45 * PI / 180.0f) * unpack_float(sendBuf + 3);
			p1Pos.y -= (float)sin(sendBuf[2] * 45 * PI / 180.0f) * unpack_float(sendBuf + 3);
			
			client_ping(client, sendBuf);
		}
		
		unsigned char* state = client_sync(client, recvBuf);
		if (state != NULL) {
			p1Pos.x = unpack_float(state + 1 + 2);
			p1Pos.y = unpack_float(state + 1 + 6);
			p2Pos.x = unpack_float(state + 1 + 10);
			p2Pos.y = unpack_float(state + 1 + 14);
		}

		DrawRectangleV(p1Pos, sz, WHITE);
		DrawRectangleV(p2Pos, sz, WHITE);
		*/

		EndDrawing();
	}

	/*
	if (client == NULL) {
		printf("No feedback from server. Closing...\n");
	}
	if (client != NULL) {
		printf("You Quit.\n");
		client_destroy(client);
	}
	*/
}