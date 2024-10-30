TARGETS = src/client/client src/server/server src/proxy/proxy

server: 
	g++ src/server/main.cpp -o src/server/server -lzmq

runServer:
	./src/server/server

proxy: 
	g++ src/proxy/main.cpp -o src/proxy/proxy -lzmq

runProxy:
	./src/proxy/proxy

client: 
	g++ src/client/main.cpp -o src/client/client -lzmq

runClient:
	./src/client/client

clean:
	rm -f $(TARGETS)