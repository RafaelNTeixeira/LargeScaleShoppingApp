TARGETS = src/client src/server src/proxy

server: 
	g++ src/server.cpp -o src/server -lzmq

proxy: 
	g++ src/proxy.cpp -o src/proxy -lzmq

client: 
	g++ src/client.cpp -o src/client -lzmq

clean:
	rm -f $(TARGETS)