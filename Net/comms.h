// We can't stop UDP sockets from receiving messages from unwanted hosts
// But we can prevent those messages from being processed with this password
#define PID 3980754

#define TIMEOUT 10

// What the client and server will be sending back to each other
union Data {
	struct {
		unsigned long pid;
		unsigned long seq;
		// unsigned long ack;
		// unsigned long bit;
	};
	unsigned char raw[8];
};

void flip(unsigned char* const bin, const unsigned short sz);
short wsa_create();
unsigned short wsa_destroy(); 