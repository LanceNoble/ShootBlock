#include "raylib.h"
#include "state.h"

#include <time.h>

static Color placeholdColor;
static int ipLen;
static char ip[16];
static Rectangle textBox;
static Rectangle button;
clock_t caretInterval;
static int isBlinking;
static int caretMode;
static Color caretColor;
static Rectangle caret;

void start_init() {

	ipLen = 1;
	ip[0] = '\0';
	textBox.width = 512;
	textBox.height = 64;
	textBox.x = WIDTH / 2 - textBox.width / 2;
	textBox.y = HEIGHT / 2 - textBox.height / 2;
	isBlinking = 0;
	caretMode = 0;
	caretColor = BLANK;
	caretInterval = 0;
	caret.width = 2;
	caret.height = textBox.height / 2;
	caret.x = textBox.x + 16;
	caret.y = textBox.y + textBox.height / 2 - caret.height / 2;
}

void start_update() {
	if (GetMouseX() >= textBox.x &&
		GetMouseX() <= textBox.x + textBox.width &&
		GetMouseY() >= textBox.y &&
		GetMouseY() <= textBox.y + textBox.height &&
		IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		isBlinking = 1;
		caretColor = WHITE;
		caretInterval = clock();
	}
	else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		isBlinking = 0;
		caretColor = BLANK;
		caretInterval = clock();
	}

	if (isBlinking && (clock() - caretInterval) * GetFrameTime() / CLOCKS_PER_SEC >= 0.003f) {
		caretInterval = clock();
		if (caretMode == 0) {
			caretMode = 1;
			caretColor = WHITE;
		}
		else {
			caretMode = 0;
			caretColor = BLANK;
		}
	}

	int keyPress = GetKeyPressed();
	if (isBlinking && ipLen > 1 && keyPress == KEY_BACKSPACE) {
		ip[ipLen - 2] = '\0';
		ipLen--;
	}
	else if (isBlinking && keyPress != 0 && keyPress != KEY_BACKSPACE && ipLen < 16) {
		ip[ipLen - 1] = keyPress;
		ip[ipLen++] = '\0';
	}

	caret.x = textBox.x + 16 + MeasureText(ip, caret.height);

	if (ipLen == 1) {
		placeholdColor = GRAY;
	}
	else {
		placeholdColor = BLANK;
	}
}

void start_draw() {
	DrawText("Game", 0, 0, 64, WHITE);
	DrawText(ip, textBox.x + 16, textBox.y + textBox.height / 2 - caret.height / 2, caret.height, WHITE);
	DrawText("Server IP Address", textBox.x + 16, textBox.y + textBox.height / 2 - caret.height / 2, caret.height, placeholdColor);
	DrawRectangleLinesEx(textBox, 4.0f, WHITE);
	DrawRectangleRec(caret, caretColor);
}