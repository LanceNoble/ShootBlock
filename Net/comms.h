#define TIMEOUT_HOST 8
#define TIMEOUT_PACKET 2

// Input Binary:
//		Byte 0:
//			Bit 0 - 1: Input Type
//				0: Move
//				1: Build
//				2: Shoot
//			Bit 2 - 4: Input Direction (only applies to move and build)
//				0: NORTH
//				1: NORTHWEST
//				2: WEST
//				3: SOUTHWEST
//				4: SOUTH
//				5: SOUTHEAST
//				6: EAST
//				7: NORTHEAST
//		Byte 1 - 4: Movement Magnitude OR Shot Angle

// State Binary:
//		Byte 0:
//			Bit 0: Connection Status
//			Bit 1: Health
//			Bit 2 - 4: Number of Bullets
//			Bit 5 - 7: Number of Blocks
//		Byte 1 - 2: Position
//		Byte 3 - 4: Bullet Position
//		

// A datagram's payload
struct Message {
	unsigned char len; // Number of bytes delivered
	char buf[0xff]; // The actual bytes
};

// Information necessary to talk to a host via UDP
struct Host {
	unsigned long ip;
	unsigned short port;
	
	long time;
	unsigned short seq;

	unsigned char numMsgs;
	struct Message* msgs;
};

union Response {
	struct { 
		unsigned long ack : 16;
		unsigned long bit : 16;
	};
	char raw[4];
};

void flip(char* const bin, const unsigned short sz);
short wsa_create();
unsigned short wsa_destroy(); 