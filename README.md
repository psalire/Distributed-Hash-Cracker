
# Distributed Hash Cracker

* [About](#about)
* [Dependencies](#dependencies)
* [Files included](#files-included)
* [How to use](#how-to-use)
    * [Steps](#steps)
        * [Server help](#server-help)
        * [Client help](#client-help)
* [Example output](#example-output)
* [Notes, assumptions, limitations](#notes-assumptions-limitations)
* [How it works](#how-it-works)
    * [Server](#server)
    * [Client](#client)
        * [Example flow](#example-flow)
* [Todo](#todo)

## About

Divides and distributes a search space among clients to brute force crack hashes, multi-threaded on the CPU.

**NOTE**: All CPU cores on your machines will be pushed to 100% utilization. Please make sure that you have adequate CPU cooling when using!

## Dependencies

```
Linux system, C++17, CryptoPP/Crypto++ (l:libcryptopp.a)

Compiler tested: g++ (Ubuntu 7.5.0-3ubuntu1~18.04) 7.5.0
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
(Compiled:
main_test   : Test executable
main_client : Client executable
main_server : Server executable)
```

## How to use

### Steps

1. Ensure dependencies are installed (above).
2. Compile and run tests with ```make test```
3. Compile client and server with ```make```
4. Run server
5. Run clients

#### Server help:
See crack_client.cc line 369 for supported hash algorithms. Also see [Todo](#todo)
```
$ ./main_server -h
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

#### Client help:
```
$ ./main_client -h
Usage: main_client -i [Server IP Address] -p [Server Port] -n [Total Threads]
-i:
    Required: IP address of server.
-p:
    Required: Port number of server.
-n:
    Required: Total threads to use.

```

## Example Output

Cracking a hash with 2 clients:

### Server with 2 clients
```
$ ./main_server -n 2 -p 8080 -a SHA512 -i 3615f80c9d293ed7402687f94b22d58e529b8cc7916f8fac7fddf7fbd5af4cf777d3d795a7a00a16bf7e7f3fb9561ee9baae480da9fe7a18769e71886b03f315
-l 5 -f -x "\`#\$%\^\*()-=\~_+[]\\|;\'\",./<>\&{}:?" -o crack_output.txt

Hash to crack       : 3615f80c9d293ed7402687f94b22d58e529b8cc7916f8fac7fddf7fbd5af4cf777d3d795a7a00a16bf7e7f3fb9561ee9baae480da9fe7a18769e71886b03f315
Hash Algorithm      : SHA512
Search space        : "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@"
Max string length   : 5
Fixed string length : True
Total clients       : 2
Port number         : 8080
Output File         : crack_output.txt

[SERVER] Accepting 2 clients...
[INFO] Accepting client #0...
[INFO] Accepted client #0
[INFO] Accepting client #1...
[INFO] Accepted client #1
[SERVER] Initializing all clients and client comms...
[SERVER] Clients are computing, waiting for results...
[SERVER] Client #1 cracked the hash.
[INFO] Client #1 CRACKED HASH: Hello
[SERVER] Client #0 finished without cracking the hash.
[SERVER] Time elapsed: 32.468s
[SERVER] Client successfully cracked 3615f80c9d293ed7402687f94b22d58e529b8cc7916f8fac7fddf7fbd5af4cf777d3d795a7a00a16bf7e7f3fb9561ee9baae480da9fe7a18769e71886b03f315
[SERVER] Saved crack to crack_output.txt.
$ cat crack_output.txt
Hello
```

### Client #1
```
$ ./main_client -i 127.0.0.1 -p 8080 -n 2

Server IP Address : 127.0.0.1
Server port       : 8080
Total Threads     : 2

[CLIENT] Connecting to server at 127.0.0.1:8080...
[INFO] Receiving settings from server...
[INFO] Hash Algo: SHA512
[CLIENT] Cracking hash...
[INFO] Cracking lengths: 5
[INFO] Cracking lengths: 5
[CLIENT] Time elapsed: 32.464s
[CLIENT] Successfully cracked the hash: Hello
[INFO] Client send results message...
```

### Client #2
```
$ ./main_client -i 127.0.0.1 -p 8080 -n 2

Server IP Address : 127.0.0.1
Server port       : 8080
Total Threads     : 2

[CLIENT] Connecting to server at 127.0.0.1:8080...
[INFO] Receiving settings from server...
[INFO] Hash Algo: SHA512
[CLIENT] Cracking hash...
[INFO] Cracking lengths: 5
[INFO] Cracking lengths: 5
[CLIENT] Time elapsed: 32.467s
[CLIENT] Finished assignment without cracking the hash.
```

## Notes, assumptions, limitations

* Macros for verbosity are defined in ```settings.h```
    * Verbose logs are printed with prefix ```[INFO]```
* Assumes server-client connection is maintained; Does not handle disconnects and reconnects

## How it works

### Server

1. Open TCP socket
2. Block for and accept connections from n clients
3. Divide search space for n clients
4. Send distributed search space and other parameters to clients
5. Wait for and receive results from clients
    * If client successfully cracked the hash, send message to all other clients to stop computing
6. If received a crack, save crack to output file

### Client

1. Open TCP socket
2. Connect to server
3. Receive parameters for cracking from server, including designated search space, hash algo, etc.
4. Brute force crack the hash based on received parameters
    * Stop computing if cracked the hash, or received signal from server to stop
5. After completion, send results to server

#### Example flow

With search space of `abcdef` and `2` clients, trying to crack a hash whose input was `abc`:

1. Server divides this search space into 2: `abc` and `def`
2. Server sends `abc` to Client #1 and `def` to Client #2
3. Client #1 attempts all strings that start with `a`, `b`, and `c`. Client #2 attempts all strings that start with `d`, `e`, and `f`.
    * E.g. Client #1 tries `a`, `aa`, `ab`, `...`, then tries `b`, `ba`, `...`, then `c`, `ca`, `...`
    * Client #2 tries `d`, `da`, `db`, `...`, then tries `e`, `ea`, `...`, then `f`, `fa`, `...`
4. Client #1 will crack the hash for `abc`, and send its results to the server
5. Client #1 sends message to Server indicating successful crack, including  the cracked string
6. Server sends message to all other clients (Client #2) that all is done and should stop computing
7. Server writes crack to file and exits
8. Clients exit

## Todo

* Test more hash algorithms (currently only SHA is implemented)
