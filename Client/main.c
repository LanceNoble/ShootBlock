
//#include "button.h"
//#include "client.h"
//#include <stdio.h>

#include <stdio.h>

#include "raylib.h"

#include "comms.h"
#include "client.h"


int main() {
	wsa_create();

	unsigned char* data;
	union Meta info;
	struct Client* client = client_create("localhost", "3490", &data, &info);

	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(1280, 800, "Hello Raylib");
	while (!(WindowShouldClose())) {
		BeginDrawing();

		ClearBackground(BLACK);

		client_sync(client);
		for (int i = 0; i < info.max; i++) {
			if (data[i * info.size] == CON_BYTE_ON) {
				DrawText(TextFormat("Player %i is Online", i), 0, i * 32, 32, WHITE);
			}
				
			else if (data[i * info.size] == CON_BYTE_OFF) {
				DrawText(TextFormat("Player %i is Offline", i), 0, i * 32, 32, WHITE);
			}
				
		}

		EndDrawing();
	}

	CloseWindow();
	client_destroy(&client);
	wsa_destroy();

	/*
	client_create();

	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(1280, 800, "Hello Raylib");

	struct Button* buttonJoin = button_create();
	button_set_text(buttonJoin, "JOIN");
	button_place(buttonJoin, 640 - button_get_width(buttonJoin) / 2, 400 - button_get_height() / 2);
	
	struct Button* buttonLeave = button_create();
	button_set_text(buttonLeave, "LEAVE");
	button_place(buttonLeave, 1280 - button_get_width(buttonLeave), 0);

	struct Player* player = NULL;
	struct Player** players = NULL;
	int numPlayers = 0;

	int joined = 0;
	int numBytes = 0;
	float spd = 250.0f;
	float xPos = 0.0f;
	float yPos = 0.0f;
	while (!WindowShouldClose()) {
		BeginDrawing();

		// Clear color and depth buffers
		ClearBackground(BLACK);

		if (joined)
			goto game;

		if (button_activate(buttonJoin)) {
			// You can't loopback external IPs without a hairpin NAT (i.e. NAT loopback)
			if (client_join("localhost") == 0) {
				joined = 1;
			}
		}

		if (!joined)
			goto end;

	game:
		numBytes = client_sync(&player, &players, &numPlayers);
		if (numBytes == 0) {
			client_leave();
			player = NULL;
			players = NULL;
			numPlayers = 0;
			joined = 0;
			xPos = 0.0f;
			yPos = 0.0f;
			goto end;
		}

		if (button_activate(buttonLeave)) {
			client_leave();
			player = NULL;
			players = NULL;
			numPlayers = 0;
			joined = 0;
			xPos = 0.0f;
			yPos = 0.0f;
			goto end;
		}

		float dt = GetFrameTime();
		if (player != NULL) {
			float x = 0;
			float y = 0;
			if (IsKeyDown(KEY_W))
				y -= spd * dt;
			if (IsKeyDown(KEY_A))
				x -= spd * dt;
			if (IsKeyDown(KEY_S))
				y += spd * dt;
			if (IsKeyDown(KEY_D))
				x += spd * dt;
			if (x != 0 || y != 0) {
				xPos += x;
				yPos += y;
				printf("notifying...\n");
				client_notify(xPos, yPos);
			}
				
		}

		for (int i = 0; i < numPlayers; i++) {
			if (players[i] == NULL)
				continue;
			unsigned int id;
			float xPos;
			float yPos;
			player_get(players[i], &id, &xPos, &yPos);

			DrawText(TextFormat("Player %i", id), 0, i * 32, 32, WHITE);
			Vector2 pos;
			pos.x = xPos;
			pos.y = yPos;
			Vector2 sz;
			sz.x = 50;
			sz.y = 50;
			DrawRectangleV(pos, sz, WHITE);
		}

	end:
		EndDrawing();
	}

	CloseWindow();
	button_destroy(&(buttonJoin));
	button_destroy(&(buttonLeave));
	client_destroy();

	return 0;
	*/
}