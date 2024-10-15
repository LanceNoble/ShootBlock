#pragma once

struct Button* button_create();
void button_set_text(struct Button* b, char* text);
int button_get_width(struct Button* b);
int button_get_height();
void button_place(struct Button* b, int x, int y);
int button_activate(struct Button* b);
void button_destroy(struct Button** b);