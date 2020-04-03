
BIN = bin
EXECS = main_server main_client main_test
CC = g++ -std=c++17 -Iinclude -Isrc -O3
FLAGS = -Wall -Wextra -Wpedantic
LINKS = -l:libcryptopp.a -pthread
REQ_OBJECTS = $(BIN)/encoder.o $(BIN)/tcp.o
ALL_OBJECTS = $(REQ_OBJECTS) $(BIN)/crack_server.o $(BIN)/crack_client.o $(BIN)/args_server.o $(BIN)/args_client.o
SERVER_OBJECTS = $(REQ_OBJECTS) $(BIN)/args_server.o $(BIN)/crack_server.o
CLIENT_OBJECTS = $(REQ_OBJECTS) $(BIN)/args_client.o $(BIN)/crack_client.o
SRC = $(wildcard src/*.cc)
HEADERS = $(wildcard include/*.h)

.PHONY: all, clean, remake, test

all: main_server main_client

### Executables ###
main_server: src/main_server.cc $(HEADERS) $(SERVER_OBJECTS)
	$(CC) -o $@ $< $(SERVER_OBJECTS) $(FLAGS) $(LINKS)

main_client: src/main_client.cc $(HEADERS) $(CLIENT_OBJECTS)
	$(CC) -o $@ $< $(CLIENT_OBJECTS) $(FLAGS) $(LINKS)

main_test: test/main_test.cc $(HEADERS) $(ALL_OBJECTS)
	$(CC) -Itest -o $@ $< $(ALL_OBJECTS) $(FLAGS) $(LINKS)

### Objects ###
bin/args_server.o: src/args_server.cc
	$(CC) -c $< $(FLAGS)
	@mv *.o $(BIN)

bin/args_client.o: src/args_client.cc
	$(CC) -c $< $(FLAGS)
	@mv *.o $(BIN)

bin/tcp.o: src/tcp.cc
	$(CC) -c $< $(FLAGS)
	@mv *.o $(BIN)

bin/%.o: src/%.cc
	$(CC) -c $< $(LINKS) $(FLAGS)
	@mv *.o $(BIN)

### Clean ###
clean:
	rm -f $(EXECS) $(wildcard bin/*.o)

# Clean and make
remake:
	make clean && make

# Make main_test and run
test:
	make main_test && ./main_test
