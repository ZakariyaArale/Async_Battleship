# Async_Battleship
This program uses asynchronous programming to build a high-performance server that manages client connections and synchronizes gameplay to simulate the mechanics of the classic game Battleship.

Requirements: Any linux terminal

How to play:
- Grid is 10x10 with the first row/col is 0 and last row/col is 9
- Ships will occupy 5 points in vertical/horizontal position using | or - 
- Use the Makefile to get the object file for server, run it
- Register by this format REG [username 20 char max alnum and '-' only] [center of ship] [ship position] [ie REG user1 2 2 | will create a ship from (0, 2) to (4, 2)]
- Bomb by this format BOMB [coordinate] [ie. BOMB 6 2 will bomb at (6,2)]
- Server will reply to client with the following msg: INVALID if the data violates the rules, TAKEN if the name has been taken, WELCOME if registration is succesful
- Server will broadcast to all registered client with the following msg:
  -   JOIN with the name of the joined user
  -   HIT with the victim, the coordinates of the attacked coordinate, and attacker
  -   MISS with corrdinate of the attacked coordiante, and attacker
  -   BYE [username] if the user is disconnected or all 5 points of the user ship is damaged and user is removed from server

 A quick way to simulate as a client local to your comuputer is nc -q 1 -v localhost 22651 in a seperate terminal [with the server running]

 Delve into the madness of multiplayer, high-stakes Battleship :)!
