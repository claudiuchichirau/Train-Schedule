# Train-Schedule
The "Train Schedule" project is an application that provides users (customer, station or train) with information about trains, their delays, etc.

## Installation & Run
1. Install WSL (Windows Subsystem for Linux) following these instructions
2. Clone the repo
3. Open two WSL windows, one for the server and one for the client, and navigate to the project directory in each of them
4. In the server WSL window, run the following command to compile the "server.cpp" file: `g++ -Wall -pthread -I/usr/include/libxml2 server.cpp -lpthread -o server -lxml2`
5. Then, run the following command to execute the compiled file: `./server`
6. In the client WSL window, run the following command to compile the "client.cpp" file: `g++ client.cpp -o client`
7. Finally, run the following command to execute the compiled file and connect to the server: `./client 127.0.0.1 3407`

## Project Description

The users can access the following information:
1. For the station user: the user can select from the following options: the arrival board, the departure board, log out, or exit the application.
2. For the client user: the user can select from the following options: the arrival board for a specific station, the departure board for a specific station, the arrivals for the next hour for a specific station, the departures for the next hour for a specific station, the train(s) that can take the client from station A to station B, log out, or exit the application.
3. For the train user: the user can select from the following options: the itinerary information for the train, update the schedule for the train, log out, or exit the application.

The program is written in C++, and it includes various libraries such as sys/types.h, sys/socket.h, arpa/inet.h, netinet/in.h, errno.h, unistd.h, stdio.h, string.h, stdlib.h, signal.h, pthread.h, cstdlib, sys/stat.h, libxml/parser.h, libxml/tree.h, time.h, and cstring.h.

