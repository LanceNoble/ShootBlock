#define TIMEOUT_HOST 600
#define TIMEOUT_PACKET 600

union Response {
	struct { 
		unsigned int ack : 16;
		unsigned int bit : 16;
	};
	unsigned char raw[4];
};