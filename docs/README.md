# A simple Documentation

This directory provides some documentation about HttpServer.

## Socket and IO

`socket_common.h` defines basic types, macros and headers used in encapsulating system-level sockets API.
- To understand why these headers/macros are defined, check the socket man page (Mac and Linux), and [Microsoft Documentation of sockets](https://docs.microsoft.com/en-us/windows/win32/winsock/winsock-server-application) (Windows)

`socket.c` implements three classes: `SocketGenerator`, `Socket`, and `SocketClient`. The first is responsible for creating a socket to listen to incoming connections. The second is responsible for listening and accepting the connection, which generates `SocketClient`.

`io.c` encapsulates read/write function on Mac and Linux, and `send/recv` function on Windows. It provides a buffered `Reader` and `Writer` for writing content to socket files.
- `io.c` defines another class `LimitSizeReader` which inherits `Reader` and provides the function to limit request size.

## Context and Task

Context and Task are an essential part of the framework.

`context.c` provides `Context` implementation which is similar to `ctx` in koa. It contains a `Request` and a `Response` object.
- In `Request`, the HTTP method, version, URI, and request headers are stored. In `Response`, response headers and content are stored.

`task.c` implements two classes `Task` and `TaskList`. `Task` encapsulates a function/functor and stores the information about the next function/functor. `TaskList` stores a list of Tasks (middleware).

`httpserver.c` implements a HttpServer class that uses a `TaskList` and a `Socket`. It allows user to register their middleware and accepts incoming connections.

## Middleware

`HeadParser` is a middleware used by `HttpServer` by default. It parses the request information and headers and stores them into the `Request` object.

## Other

`servererrors` defines and implements a list of error codes and their human-readable meaning.
- To see how to define and extend the error codes and messages, read [Creating your own error conditions](http://blog.think-async.com/2010/04/system-error-support-in-c0x-part-5.html)