#include <Windows.h>

#include "comms.h"
#include "client.h"

#include <stdio.h>

int main() {
	short res = wsa_create();
	if (res != 0) {
		printf("WSA Init Fail\n");
		return res;
	}
	
	void* client = client_create("127.0.0.1", 3490);
	/*
	while (0) {
		struct Message test;
		test.len = 3;
		test.buf[0] = 'a';
		test.buf[1] = 'b';
		test.buf[2] = 'c';
		client_ping(client, test);
	}
	*/
	
	for (unsigned long i = 0; i < 16; i++) {
		struct Message test;
		test.len = 3;
		test.buf[0] = 'a';
		test.buf[1] = 'b';
		test.buf[2] = 'c';
		client_ping(client, test);
	}

	/*
	struct Client* client;
	res = client_create(127, 0, 0, 1, 3490, &client);
	if (res != 0) {
		printf("Connection Fail\n");
		client_destroy(&client);
		wsa_destroy();
		return res;
	}
	
	while (!(GetAsyncKeyState(VK_END) & 0x01)) {
		client_ping(client);
		if (client_sync(client) == -1) {
			printf("Server Died\n");
			break;
		}
		
	}
	*/

	client_destroy(client);
	wsa_destroy();
}