// ShitNet: a shitty netcode library
//
// This was built on some general ideas I had on how multiplayer works.
// These generalizations don't apply to all games.
// But they're simple enough for basic netcode.
// 
// Cycle:
// 1. Client A does something
// 2. Client A pings server they did something
// 3. Server pings all clients (even Client A) that Client A did something
// 4. All clients (even Client A) must reflect this event in their game
//
// As for what data should be pinged exactly, that should be up to the user.
// So I looked at the bigger picture:
// 1. Multiplayer is just the client and server having the same player data.
// 2. We can think of that data as just a bunch of bytes that the client and server have their own version of
// 
// Thus, this library will give the user the tools to give meaning to that bunch of bytes.
// However, the user will not have full control over all bytes:
// 1. The library will divide these bytes equally so that every player has an equal amount of bytes
// 2. The library will be reserve the first byte of every player to represent their connection status
// 
// I felt that the user shouldn't control these 2 things for 2 reasons:
// 1. One player having more bytes than another is unfair and doesn't make sense.
// 2. ...Why wouldn't you track the connection status of a player...?
// 
// Now all that was left to do was put restrictions on 2 more things:
// 1. The maximum number of players the user could configure a server to house
// 2. The number of bytes each player will have
// 
// This is ShitNet, soooo the user will only be able to make multiplayer games of up to 4 players.
// And they only have 16 bytes to represent a player.
//
// TODO:
// Oh yeah and there's no functionality for syncing server-wide events (e.g. a zombie spawned)
// Doesn't make sense to sync that in the player data
// So for future reference, I was thinking of adding just another buncha bytes dedicated towards those events
// 
// Also provide functionality for specifying the operation to perform on the stat
// (e.g. set equal to? divide? multiply? add? subtract)
// 

#pragma once

#define SERVER_MAX 4 // Max number of players the user can configure a server can house
#define PLAYER_SIZE 16 // Number of bytes reserved to represent a player's stats
#define SYNC_SIZE 64 // SERVER_MAX * PLAYER_SIZE
#define MAX_STAT_SIZE 4 // Max player stat size

// First byte of a player's data is reserved for representing their connection status to the server
#define CON_BYTE 0
#define CON_BYTE_SIZE 1
#define CON_BYTE_OFF 0b00000000
#define CON_BYTE_ON 0b11111111

// The message the client will receive from the server upon joining
union Meta {
	struct {
		
		// Which player the client is
		unsigned char you : 2;

		// Server's player max
		// Recipients must offset this value by +1 since a value of 0 can be sent
		unsigned char max : 2;

		// How many bytes this server is using to represent a player
		// Recipients must offset this value by +1 since a value of 0 can be sent
		unsigned char size : 4;
	};

	unsigned char raw[1];
};

// The message both the server and client will send to each other for syncing
union Bump {
	struct {

		// Server list index of the player to modify the bytes of
		unsigned char id : 2;
		
		// Index of where to start copying data in the player's data bytes
		unsigned char start : 4;

		// Size of data
		// Recipients must offset this value by +1 since a value of 0 can be sent
		unsigned char size : 2;

		// Actual new value of the stat (in bytes)
		unsigned char value[MAX_STAT_SIZE];
	};

	unsigned char raw[1 + MAX_STAT_SIZE];
};

void flip(unsigned char* const bytes, const unsigned short sz);
short wsa_create();
unsigned short wsa_destroy();