CC=gcc 
CFLAGS=-Wall
INCLUDE=-Iinclude
SOURCES=main.c lgtv.c sds.c
LIBS=-lcurl
EXECUTABLE=lgCommander

all:
	$(CC) $(CFLAGS) $(INCLUDE) $(SOURCES) -o $(EXECUTABLE) $(LIBS)
clean:
	rm -vf $(EXECUTABLE)


