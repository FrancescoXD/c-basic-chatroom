# c-basic-chatroom
A basic chatroom fully written in C with sockets and poll

## How to compile
Just run:
```
$ chmod +x compile.sh
$ ./compile.sh
```

## Running
Open a terminal and run `./server <HOST> <PORT>` and then run clients with `./client <HOST> <PORT>`.

## Info
If no client try to connect to the server in 3 minutes, it exits.