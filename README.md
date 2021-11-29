This is a primitive implementation of multiplayer NIM Game using socket programming following TCP protocol.
Game allows multiple users to connect to a server and play with another player in order in which they started a game.


If you want to learn the details of NIM please visit : https://en.wikipedia.org/wiki/Nim

Compile Instructions:

If you want to compile the programs , you can do so by typing the following:

	gcc nimProtocol.c nimServer.c -lpthread -o nimServer
	gcc nimProtocol.c nimClient.c -o nimClient

Run Instructions: 

To run the server type:

	./nimServer [PORT NUMBER] [SIZE OF FIRST HEAP] [SIZE OF SECOND HEAP] [SIZE OF THIRD HEAP]
	
[PORT NUMBER]: The port on which the server is accepting requests. eg. 544321

To run a client type:

	./nimClient  [CLIENT NAME] [SERVER NAME] [PORT NUMBER]

[CLIENT NAME]: Short nickname. 
[SERVER NAME]: The server hostname.
[PORT NUMBER]: The port where the server is running.


Example:
	./nimServer 544321 9 8 9
	./nimClient mark 127.0.0.1 544321 
