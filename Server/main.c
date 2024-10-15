//#include <stdio.h>
//#include <Windows.h>

//#include "server.h"
#include "mem.h"

int main() {
	mem_track_create();

	int* killme = DEBUG_MALLOC(sizeof(int));
	DEBUG_FREE(killme, 1);

	mem_track_show();

	mem_track_destroy();

	return 1;
	/*
	server_create();
	server_start();

	while (!(GetAsyncKeyState(VK_END) & 0x01)) {
		server_greet();
		server_sync();
	}

	server_stop();
	server_destroy();
	*/
}