#pragma once

#include "common.h"
#include "csr/option.hpp"
#include "http/context.h"
#include "http/task.h"
#include "socket/socket.h"

class HttpServer {
private:
  Socket s;
  TaskList tasklist;

public:
  HttpServer(int port);
  ~HttpServer() = default;

  NOT_COPYABLE(HttpServer);
  NOT_MOVEABLE(HttpServer);

  HttpServer &use(std::function<void(Context &, const Task &)> &&f);
  void run() const;
};

class HttpClient {
private:
  Context ctx;
  SocketClient sc;

public:
  HttpClient(SocketClient &&sc);
  ~HttpClient() = default;

  NOT_COPYABLE(HttpClient);
  NOT_MOVEABLE(HttpClient);

  void start(const Task &task);
};