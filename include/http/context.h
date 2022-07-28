#pragma once

#include "common.h"
#include "csr/result.hpp"
#include "servererrors.h"
#include "socket/socket_common.h"
#include <map>
#include <string>
#include <vector>

struct Request {
  std::string method;
  std::string version;
  std::string path, fullpath;
  std::map<std::string, std::string> params;
  std::map<std::string, std::string> headers;
  std::vector<char> content;

  void setContent(const std::string &s);
  void setContent(std::vector<char> &&v);
};

struct Response {
  std::string status;
  std::map<std::string, std::string> headers;
  std::vector<char> content;

  void setContent(const std::string &s);
  void setContent(std::vector<char> &&v);
};

class Context {
private:
  m_sock_t fd;

public:
  Request req;
  Response resp;

  // ? add payload here
private:
  Context(m_sock_t fd);
  ~Context() = default;

  NOT_COPYABLE(Context);
  NOT_MOVEABLE(Context);

public:
  void write();

  friend class HttpClient;
  friend class HeadParser;
};