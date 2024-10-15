#pragma once

// Some RayLib functions have the same name as Windows functions, 
// So we can't include RayLib and Windows in the same source file
// So we simply write our client in a separate source file
// And link its functions to our main source file via a header file

int client_create();
int client_join(char* ip);
int client_sync(struct Player** p, struct Player*** ps, unsigned int* numPlayers);
void client_notify(float x, float y);
void client_leave();
void client_destroy();
