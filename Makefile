all: client server

client:
	g++ -I. client/client.cpp -o client/client

server:
	g++ -I. server/server.cpp -o server/server

clean:
	rm -f client/client server/server

.PHONY: all client server clean
