#define MAX_PLAYERS 2

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

// Input Binary Prototype:
//		Byte 0:
//			Bit 0 - 2: Input Direction
//				2: NORTH
//				7: NORTHWEST
//				4: WEST
//				1: SOUTHWEST
//				6: SOUTH
//				3: SOUTHEAST
//				0: EAST
//				5: NORTHEAST
//		Byte 1 - 4: Movement Magnitude

// 0 % 8 = 0
// 225 % 8 = 1
// 90 % 8 = 2
// 315 % 8 = 3
// 180 % 8 = 4
// 45 % 8 = 5
// 270 % 8 = 6
// 135 % 8 = 7

// 0 / 45 = 0;
// 45 / 45 = 1;
// 90 / 45 = 2;
// 135 / 45 = 3;
// 180 / 45 = 4;
// 225 / 45 = 5
// 270 / 45 = 6;
// 315 / 45 = 7;

// 315 / 8 = 39 + 3

// State Binary:
//		Byte 0:
//			Bit 0: Connection Status
//			Bit 1: Health
//			Bit 2 - 4: Number of Bullets
//			Bit 5 - 7: Number of Blocks
//		Byte 1 - 2: Position
//		Byte 3 - 4: Bullet Position
//		

// State Binary Prototype:
// 8 Bytes per Player:
//		Per Player:
//			4 Bytes (x and y coord)
//		Bytes 0 - 1: Position

// A datagram's payload
struct Message {
	int len; // Number of bytes delivered
	unsigned char buf[32]; // The actual bytes
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