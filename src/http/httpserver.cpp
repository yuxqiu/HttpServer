#include "http/httpserver.h"
#include "middleware/headparser/headparser.h"
#include "socket/io.h"
#include <thread>
#include <vector>

HttpServer::HttpServer(int port)
    : s(std::move(SocketGenerator::listen(port).unwrap())) {
  use(HeadParser());
}

static void process_req(SocketClient &&sc, Task *task) {
  HttpClient client{std::move(sc)};
  client.start(*task);
}

void HttpServer::run() const {
  while (true) {
    std::thread t{process_req, std::move(s.accept().unwrap()), tasklist.head()};
    t.detach();
  }
}

HttpServer &HttpServer::use(std::function<void(Context &, const Task &)> &&f) {
  tasklist.use(std::move(f));
  return *this;
}

HttpClient::HttpClient(SocketClient &&sc)
    : ctx(sc.connfd.unwrap()), sc(std::move(sc)) {}

void HttpClient::start(const Task &task) {
  task.next(ctx);
  ctx.write();
}