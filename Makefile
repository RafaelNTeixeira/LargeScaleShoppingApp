PHONY: server runServer proxy runProxy runClient test testCRDT clean
TARGETS = src/client/client src/server/server src/proxy/proxy test/crdt/test

server: 
	g++ src/server/main.cpp -o src/server/server -lzmq

runServer:
	./src/server/server

proxy: 
	g++ src/proxy/main.cpp -o src/proxy/proxy -lzmq

runProxy:
	./src/proxy/proxy

client: 
	g++ src/client/main.cpp -o src/client/client -lzmq -luuid -lsqlite3

runClient:
	./src/client/client

testCRDT: test/crdt/test.cpp src/crdt/*
	g++ test/crdt/test.cpp -o test/crdt/test -O3 -lgtest -lgtest_main -pthread

test: testCRDT
	./test/crdt/test

clean:
	rm -f $(TARGETS)