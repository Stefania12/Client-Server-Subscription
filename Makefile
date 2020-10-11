CFLAGS = -Wall -g
PORT = 5000
IP_SERVER = 127.0.0.1

build:
	g++ server.cpp UDPManager.cpp NotificationValidityChecker.cpp TCPManager.cpp -g -o server $(CFLAGS)
	g++ subscriber.cpp NotificationValidityChecker.cpp NotificationInterpreter.cpp -g -o subscriber $(CFLAGS)
run_server:
	./server ${PORT}
clean:
	rm server subscriber
