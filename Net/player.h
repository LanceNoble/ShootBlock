#pragma once

struct Player* player_create(unsigned int id, float xPos, float yPos);
void player_get(struct Player* p, unsigned int* id, float* xPos, float* yPos);
void player_place(struct Player* p, float x, float y);
void player_destroy(struct Player** p);