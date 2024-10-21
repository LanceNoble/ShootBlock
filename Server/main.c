#include <Windows.h>
#include <stdio.h>

#include "comms.h"
#include "server.h"

int main() {
	wsa_create();
	struct Server* server = server_create(3, 1, "3490");

	while (!(GetAsyncKeyState(VK_END) & 0x01)) {
		server_relay(server);
	}

	server_destroy(&server);
	wsa_destroy();
}