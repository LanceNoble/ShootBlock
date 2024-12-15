#include "state.h"

static void (*update)();
static void (*draw)();

void state_change(void (*newUpdate)(), void (*newDraw)()) {
	update = newUpdate;
	draw = newDraw;
}

void state_invoke() {
	(*update)();
	(*draw)();
}