#include <Windows.h>

#include "comms.h"
#include "client.h"

int main() {
	wsa_create();
	struct Client* client = client_create(127, 0, 0, 1, 3490);
	
	while (!(GetAsyncKeyState(VK_END) & 0x01)) {
		client_ping(client);
		client_sync(client);
	}

	if (client != NULL) {
		client_destroy(&client);
	}
	wsa_destroy();
}