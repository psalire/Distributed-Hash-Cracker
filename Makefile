
BIN = bin
EXECS = main main_test
CC = g++ -std=c++17 -Iinclude -Isrc -O3
FLAGS = -Wall -Wextra -Wpedantic
LINKS = -l:libcryptopp.a -pthread
OBJECTS = $(BIN)/encoder.o $(BIN)/crack_server.o $(BIN)/crack_client.o $(BIN)/tcp.o $(BIN)/args.o
SRC = $(wildcard src/*.cc)
HEADERS = $(wildcard include/*.h)

.PHONY: all, clean, remake, test

all: main

### Executables ###
main: src/main.cc $(HEADERS) $(OBJECTS)
	$(CC) -o $@ $< $(OBJECTS) $(FLAGS) $(LINKS)

main_test: test/main_test.cc $(HEADERS) $(OBJECTS)
	$(CC) -Itest -o $@ $< $(OBJECTS) $(FLAGS) $(LINKS)

### Objects ###
bin/args.o: src/args.cc
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
