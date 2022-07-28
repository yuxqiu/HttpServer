# HttpServer

HttpServer is a cross-platform multi-threaded HttpServer written on C++.

It is inspired by CSAPP and the koa framework for Node.js.

# Installation

1. Clone the repository
2. Place your source file into the `src` folder.
3. Include header `#include "http/httpserver.h"`

# Usage

## HelloWorld

To write a simple HttpServer that prints HelloWorld, you can create a file like this:

```c++
#include "http/httpserver.h"

int main(int argc, char **argv) {
  int port = 8000;

  HttpServer http{port};
  http.use([](Context &ctx, const Task &next) {
        ctx.resp.status = "200";
        ctx.resp.headers["Content-Type"] = "text/plain";
        ctx.resp.setContent("Hello World!");
        next.drop();
      })
      .run();

  return 0;
}
```

## Middleware

Functions registered via `http.use()` are known as middleware. A collection of middleware is a sequence of actions the server does before delivering a response to the client.

The order of execution of middleware follows the First Register First Execute principle.

To register a middleware, you should create a function/functor that takes two parameters: `Context&` and `const Task&`.

The `Context` class includes two subclasses: one for `Request` and the other for `Response`. The `Task` class stores the function/functor that needs to be called.

To respond to the client, you should modify `Response` to provide the appropriate headers and content. To run the next function/functor in the action chain, `task.next()` needs to be called.
- If no action is needed, call `task.drop()`.

See [docs](. /docs/) for more information about `Context`, `Task` and the Socket API...

# Todo

1. Use `epoll` to write an event-based server
2. Implement middleware: 1) router and 2) static file sharing

# License

[MIT License](./LICENSE)