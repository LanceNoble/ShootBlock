#include "raylib.h"
#include "button.h"
#include <stdlib.h>

#define FONT_SIZE 64
#define PADDING 64

struct Button {
	int x;
	int y;
	char* text;
};
struct Button* button_create() {
	struct Button* b = malloc(sizeof(struct Button));
	if (b == NULL)
		return NULL;
	return b;
}
void button_set_text(struct Button* b, char* text) {
	b->text = text;
}
int button_get_width(struct Button* b) {
	return MeasureText(b->text, FONT_SIZE) + PADDING;
}
int button_get_height() {
	return FONT_SIZE;
}
void button_place(struct Button* b, int x, int y) {
	b->x = x;
	b->y = y;
}
int button_activate(struct Button* b) {
	DrawRectangle(b->x, b->y, button_get_width(b), button_get_height(), WHITE);
	DrawText(b->text, b->x + PADDING / 2, b->y, FONT_SIZE, BLACK);
	if (GetMouseX() > b->x &&
		GetMouseX() < b->x + button_get_width(b) &&
		GetMouseY() > b->y &&
		GetMouseY() < b->y + button_get_height(b) &&
		IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
		return 1;
	return 0;
}
void button_destroy(struct Button** b) {
	free(*b);
	*b = NULL;
}