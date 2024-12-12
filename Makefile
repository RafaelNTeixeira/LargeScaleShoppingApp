.PHONY: database worker runWorker1 runWorker2 broker runBroker runClient test testCRDT clean
TARGETS = database/cloud/database.db database/local/* src/client/client src/worker/worker src/broker/broker test/crdt/test database/local/shopping_lists.db
SYSTEM_PACKAGES = $(shell cat system-requirements.txt)

all: clean database worker broker client

system-requirements:
	sudo apt-get update
	sudo apt-get install -y $(SYSTEM_PACKAGES)

worker: 
	g++ -std=c++17 src/worker/main.cpp -o src/worker/worker -lzmq -luuid -lsqlite3 -pthread

runWorker1:
	./src/worker/worker "tcp://localhost:5555" "tcp://*:5558" "5601" ""

runWorker2:
	./src/worker/worker "tcp://localhost:5555" "tcp://*:5559" "5602" "tcp://localhost:5601"

runWorker3:
	./src/worker/worker "tcp://localhost:5555" "tcp://*:5560" "5603" "tcp://localhost:5602"

broker: 
	g++ -std=c++17 src/broker/main.cpp -o src/broker/broker -lzmq

runBroker:
	./src/broker/broker

client: 
	g++ -std=c++17 src/client/main.cpp -o src/client/client -lzmq -luuid -lsqlite3 -pthread

runClient:
	./src/client/client

testCRDT: test/crdt/test.cpp src/crdt/*
	g++ test/crdt/test.cpp -o test/crdt/test -O3 -lgtest -lgtest_main -pthread

test: testCRDT
	./test/crdt/test

database:
	sqlite3 database/cloud/database.db < database/cloud/database.sql

clean:
	rm -f $(TARGETS)