# CS3377-04NamedPipesGuessWords
In Linux:
Use the standard file operations for the named pipes – open(), read(), write(), close() … etc. Do not use FILE and the f_ file commands (file descriptors) for any file operations. 
 
Use C++ (g++) features (iostream, string… etc.). Build using g++ -std=c++11. 
 
The names of the source programs and associated execution files are: 
gserver.cpp gserver gclient.cpp gclient 
 
Program Command Line 
 
To run the game: 
./gserver words.txt & 
./gclient  
The words.txt source will contain words that are used to pick out a random word. 
 
The server waits for a client connection across a known named pipe that is known to both client and server.  
When the server and client is done running one game, both the client and server must exit. 
 
All created named pipes must be in the development directory.   
 
The programs must close and unlink all created pipes when done with a game. 
