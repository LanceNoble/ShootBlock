#pragma once

#define WIDTH 1280
#define HEIGHT 720

void state_change(void (*newUpdate)(), void (*newDraw)());
void state_invoke();