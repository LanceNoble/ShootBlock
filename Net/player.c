#include "player.h"
#include "stdlib.h"

struct Player {
	unsigned int id;
	float xPos;
	float yPos;
};
struct Player* player_create(unsigned int id, float xPos, float yPos) {
	struct Player* p = malloc(sizeof(struct Player));
	if (p == NULL)
		return NULL;

	p->id = id;
	p->xPos = xPos;
	p->yPos = yPos;

	return p;
}
void player_get(struct Player* p, unsigned int* id, float* xPos, float* yPos) {
	if (id != NULL)
		*id = p->id;
	if (xPos != NULL)
		*xPos = p->xPos;
	if (yPos != NULL)
		*yPos = p->yPos;
}
void player_place(struct Player* p, float x, float y) {
	p->xPos = x;
	p->yPos = y;
}
void player_destroy(struct Player** p) {
	free(*p);
	*p = NULL;
}