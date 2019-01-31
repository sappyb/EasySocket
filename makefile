CC=g++
CFLAGS=-g -W -std=c++11

all: messenger_server.x messenger_client.x
	

messenger_server.x: server.cpp
	$(CC) $(CFLAGS) -o messenger_server.x server.cpp

messenger_client.x: client.cpp
	$(CC) $(CFLAGS) -lpthread -o messenger_client.x client.cpp

clean:
	rm -f server
	rm -f client
	rm -f *.x
	rm -f *~
	rm -f core
