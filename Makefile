.PHONY: database worker runWorker1 runWorker2 runWorker3 runWorker4 broker runBroker runClient test testCRDT clean
TARGETS = database/cloud/*/* database/local/*/* src/client/client src/worker/worker src/broker/broker test/crdt/test database/local/shopping_lists.db
SYSTEM_PACKAGES = $(shell cat system-requirements.txt)

all: clean database worker broker client

system-requirements:
	sudo apt-get update
	sudo apt-get install -y $(SYSTEM_PACKAGES)

worker: 
	g++ -std=c++17 src/worker/main.cpp -o src/worker/worker -lzmq -luuid -lsqlite3 -pthread

runWorker:# with arguments from the command line
	./src/worker/worker $(ARGS)
runWorker1:
	mkdir -p database/cloud/1/
	./src/worker/worker "tcp://localhost:5555" "tcp://localhost:5558" "tcp://*:5601" "" "database/cloud/1"

runWorker2:
	mkdir -p database/cloud/2/
	./src/worker/worker "tcp://localhost:5555" "tcp://localhost:5558" "tcp://*:5602" "tcp://localhost:5601" "database/cloud/2"

runWorker3:
	mkdir -p database/cloud/3/
	./src/worker/worker "tcp://localhost:5555" "tcp://localhost:5558" "tcp://*:5603" "tcp://localhost:5602" "database/cloud/3"

runWorker4:
	mkdir -p database/cloud/4/
	./src/worker/worker "tcp://localhost:5555" "tcp://localhost:5558" "tcp://*:5604" "tcp://localhost:5603" "database/cloud/4"

runWorker5:
	mkdir -p database/cloud/5/
	./src/worker/worker "tcp://localhost:5555" "tcp://localhost:5558" "tcp://*:5605" "tcp://localhost:5604" "database/cloud/5"

runWorker6:
	mkdir -p database/cloud/6/
	./src/worker/worker "tcp://localhost:5555" "tcp://localhost:5558" "tcp://*:5606" "tcp://localhost:5605" "database/cloud/6"

runWorker7:
	mkdir -p database/cloud/7/
	./src/worker/worker "tcp://localhost:5555" "tcp://localhost:5558" "tcp://*:5607" "tcp://localhost:5606" "database/cloud/7"

runWorker8:
	mkdir -p database/cloud/8/
	./src/worker/worker "tcp://localhost:5555" "tcp://localhost:5558" "tcp://*:5608" "tcp://localhost:5607" "database/cloud/8"

broker: 
	g++ -std=c++17 src/broker/main.cpp -o src/broker/broker -lzmq

runBroker:
	./src/broker/broker

client: 
	g++ -std=c++17 src/client/main.cpp -o src/client/client -lzmq -luuid -lsqlite3 -pthread

runClient1:
	mkdir -p database/local/user1/
	./src/client/client "tcp://localhost:5556" "database/local/user1/"

runClient2:
	mkdir -p database/local/user2/
	./src/client/client "tcp://localhost:5556" "database/local/user2/"

compileTestCRDT: test/crdt/test.cpp src/crdt/*
	g++ test/crdt/test.cpp -o test/crdt/test -O3 -lgtest -lgtest_main -pthread

compileTestDatabase: test/database/test.cpp src/database.h
	g++ test/database/test.cpp -o test/database/test -O3 -lgtest -lgtest_main -pthread

testCRDT: compileTestCRDT
	./test/crdt/test

testDatabase: compileTestDatabase
	./test/database/test
	
test: testCRDT testDatabase

clean:
	rm -f $(TARGETS)