.PHONY: database worker runWorker broker runBroker runClient test testCRDT clean
TARGETS = database/cloud/database.db database/local/* src/client/client src/worker/worker src/broker/broker test/crdt/test database/local/shopping_lists.db
SYSTEM_PACKAGES = $(shell cat system-requirements.txt)

all: clean database worker broker client 

system-requirements:
	sudo apt-get update
	sudo apt-get install -y $(SYSTEM_PACKAGES)

worker: 
	g++ -std=c++17 src/worker/main.cpp -o src/worker/worker -lzmq -lsqlite3 -pthread

runWorker:
	./src/worker/worker

broker: 
	g++ -std=c++17 src/broker/main.cpp -o src/broker/broker -lzmq

runBroker:
	./src/broker/broker

client: 
	g++ -std=c++17 src/client/main.cpp -o src/client/client -lzmq -luuid -lsqlite3 -pthread

runClient:
	./src/client/client

compileTestCRDT: test/crdt/test.cpp src/crdt/*
	g++ test/crdt/test.cpp -o test/crdt/test -O3 -lgtest -lgtest_main -pthread

compileTestDatabase: test/database/test.cpp src/database.h
	g++ test/database/test.cpp -o test/database/test -O3 -lgtest -lgtest_main -pthread

testCRDT: compileTestCRDT
	./test/crdt/test

testDatabase: compileTestDatabase
	./test/database/test
	
test: testCRDT testDatabase

database:
	sqlite3 database/cloud/database.db < database/cloud/database.sql

clean:
	rm -f $(TARGETS)