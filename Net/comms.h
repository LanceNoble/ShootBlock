#define PID 3980754
#define TIMEOUT 10

union Data {
	struct {
		unsigned long pid;
	};
	unsigned char raw[4];
};

void flip(unsigned char* const bytes, const unsigned short sz);
short wsa_create();
unsigned short wsa_destroy();