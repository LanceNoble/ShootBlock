#include "raylib.h"

#include "convert.h"
#include "comms.h"
#include "client.h"

#include <math.h>
#include <stdio.h>

#define PI 3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067

int main() {
	struct Client* client = client_create("73.119.107.1", 3490);
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(1280, 800, "ShootBlock");

	char buf[1024];
	char input[7];
	int i = 0;

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

			float spd = 500.0f;
			input[2] = (int)(deg / 45);
			
			pack_float(spd * GetFrameTime(), input + 3);
			p1Pos.x += (float)cos(input[2] * 45 * PI / 180) * unpack_float(input + 3);
			p1Pos.y -= (float)sin(input[2] * 45 * PI / 180) * unpack_float(input + 3);
			client_ping(client, input);
		}
		
		unsigned char* state = client_sync(client, buf);
		if (state != NULL) {
			p1Pos.x = unpack_float(state + 1 + 2);
			p1Pos.y = unpack_float(state + 1 + 6);
			p2Pos.x = unpack_float(state + 1 + 10);
			p2Pos.y = unpack_float(state + 1 + 14);
		}

		DrawRectangleV(p1Pos, sz, WHITE);
		DrawRectangleV(p2Pos, sz, WHITE);
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