# Tasks
- Detect intentional disconnections from server
# Issues
- Sending state every server tick breaks the game
  - Reason:
    - `client_sync` won't return until there's no more messages to receive
    - Its buffer can't hold all the states the server sent since the last `client_sync` call
  - Solutions:
    1. Bigger buffer (current choice)
    2. Limit the number of messages `client_sync` can read in one call
    3. Lower the rate at which the server sends state

# ShootBlock
- 2D PvP (2 - 4 Players) 
- Goal: Kill other players
- Mechanics:
  - WASD: Move
  - MouseMove: Aim
  - RightClick: Place Block
  - LeftClick: Fire
    - Bullets bounce off of any obstacle (blocks, walls, players)
  - Space: Dash
## MVP
- One unchanging arena: an empty box with a white outline (which represents the walls)
  - Available players will spawn near the four corners from left to right then top to bottom
- Bullets are NOT hitscan
  -  
## Netcode
