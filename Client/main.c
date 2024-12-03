//#include <Windows.h>

#include "raylib.h"

#include "convert.h"
#include "comms.h"
#include "client.h"

#include <math.h>
#include <stdio.h>

int main() {
	short res = wsa_create();
	if (res != 0) {
		printf("WSA Init Fail\n");
		return res;
	}
	
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
			ping.buf[0] = deg % 8;
			float dt = GetFrameTime();
			//printf("moving %i deg at %f mag", deg, spd * dt);
			union PackedFloat pf;
			pf.pack = pack_float(spd * GetFrameTime());
			ping.buf[1] = pf.raw[0];
			ping.buf[2] = pf.raw[1];
			ping.buf[3] = pf.raw[2];
			ping.buf[4] = pf.raw[3];
			client_ping(client, ping);
			/*
			union PackedFloat newPF;
			newPF.raw[0] = pf.raw[0];
			newPF.raw[1] = pf.raw[1];
			newPF.raw[2] = pf.raw[2];
			newPF.raw[3] = pf.raw[3];
			printf(" new mag: %f\n", unpack_float(newPF.pack));
			*/
		}
		/*
		struct Message* state = client_sync(&client);
		*/

		EndDrawing();
	}

	if (client == NULL) {
		printf("No feedback from server. Closing...\n");
	}
	if (client != NULL) {
		printf("You Quit.\n");
		client_destroy(client);
	}
	wsa_destroy();
}