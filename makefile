PROG = server
CC = g++
CPPFLAGS = -Wall
OBJS = server.o base64.o http.o sha1.o utils.o websocket.o
HEADERS = base64.h http.h sha1.h utils.h websocket.h

$(PROG) : $(OBJS)
	$(CC) $(CPPFLAGS) -o $(PROG) $(OBJS)

server.o : http.h sha1.h websocket.h server.cpp
	g++ $(CPPFLAGS) -c server.cpp
base64.o : base64.h base64.c
	g++ $(CPPFLAGS) -c base64.c
http.o : http.h http.cpp
	g++ $(CPPFLAGS) -c http.cpp
sha1.o : sha1.h sha1.c
	g++ $(CPPFLAGS) -c sha1.c
utils.o : utils.h utils.cpp
	g++ $(CPPFLAGS) -c utils.cpp
websocket.o : websocket.h base64.h sha1.h websocket.c
	g++ $(CPPFLAGS) -c websocket.c

clean:
	rm -f $(OBJS)


