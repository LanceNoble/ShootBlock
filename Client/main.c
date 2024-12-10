#include "raylib.h"

#include "convert.h"
#include "comms.h"
#include "client.h"

#include <math.h>
#include <stdio.h>

void main() {
	struct Client* client = client_create("73.119.107.1", 3490);
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(1280, 800, "ShootBlock");

	unsigned char buf[4096];
	unsigned char input[7];

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

			input[2] = (unsigned char)(deg / 45);
			//printf("%i\n", input[2]);
			pack_float(500.0f * GetFrameTime(), input + 3);
			
			p1Pos.x += (float)cos(input[2] * 45 * PI / 180.0f) * unpack_float(input + 3);
			p1Pos.y -= (float)sin(input[2] * 45 * PI / 180.0f) * unpack_float(input + 3);
			
			client_ping(client, input);
		}
		
		unsigned char* state = client_sync(client, buf);
		if (state != NULL) {
			printf("Prediction: %f, %f\n", p1Pos.x, p1Pos.y);

			p1Pos.x = unpack_float(state + 1 + 2);
			p1Pos.y = unpack_float(state + 1 + 6);

			printf("Actual: %f, %f\n\n", p1Pos.x, p1Pos.y);

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