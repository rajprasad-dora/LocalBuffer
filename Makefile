all: server

server:
	g++ -I. server/server.cpp -o server/server

clean:
	rm -f server/server

run-server:
	sudo ./server/server

run-client:
	./server/server 172.17.0.2 -c

run-client-latest:
	./server/server 172.17.0.2 -c -l

run-print-file:
	cat /app/filesToShare/file1.txt

.PHONY: all server clean run-server run-client run-client-latest
