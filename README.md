
# Distributed Hash Cracker

## About

Distributes search space among clients to brute force crack hashes.

## Dependencies

```
Linux system, C++17, CryptoPP/Crypto++ (l:libcryptopp.a)

Compiler tested: gcc (Ubuntu 7.4.0-1ubuntu1~18.04.1) 7.4.0
```

## Files included

```
bin/
    All compiled .o files
include/
    All .h header files
src/
    args_server.cc  : Argparse for main_server.cc
    args_client.cc  : Argparse for main_client.cc
    crack_client.cc : Classes for creating the client
    crack_server.cc : Class for creating the server
    encoder.cc      : Class for encoding hex to bytes
    main_client.cc  : Client main
    main_server.cc  : Server main
    tcp.cc          : Class for TCP socket comms
test/
    main_test.cc    : Testing program main
Makefile
main_test   : Test executable
main_client : Client executable
main_server : Server executable
```

## How it works

## Server

1. Open TCP socket
2. Block for and accept connections from n clients
3. Divide search space to distribute to all clients
4. Send search space and other info to clients
5. Wait for clients to complete
6. Receive results from clients
    * If client successfully cracked the hash, send message to all other clients to stop computing
7. If received a crack, save crack to output file

## Client

1. Open TCP socket
2. Connect to server
3. Receive parameters for cracking from server, including designated search space, hash algo, etc.
4. Brute force crack the hash based on parameters from server
    * Stop computing if cracked the hash, or received signal from server to stop
5. After completion, send results to server

## How to use

### Steps

1. Ensure dependencies are installed (above).
2. Compile and run tests with ```make test```
3. Compile client and server with ```make```

### Server help:
```
> ./main_server -h
Usage: main_server -i [Checksum String] -a [Hash Algorithm] -n [Total Clients] -p [Port Number] -o [Output File]
-i:
    Required: Hash/checksum to crack.
-a:
    Required: Hash algorithm to use. See README for supported inputs. 
-n:
    Required: Total clients to accept.
-p:
    Required: Port number to create server on.
-o:
    Required: Output file to save cracked result.
-s:
    Optional: Set search space.
    Default:
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789`!@#$%^&*()-=~_+[]\{}|;':",./<>?"
-x:
    Optional: Exclude/remove characters from search space.
-c:
    Optional: Include/add characters to search space. Duplicates ignored. 
-l:
    Optional: Set a max string length for cracking.
    Default: 0, meaning no max; try all possible string lengths.
-f:
    Optional: Fixed string length; Only crack strings of length max string length.
```

### Client help:
```
> ./main_client -h
Usage: main_client -i [Server IP Address] -p [Server Port] -n [Total Threads] 
-i:
    Required: IP address of server.
-p:
    Required: Port number of server.
-n:
    Required: Total threads to use.

```
