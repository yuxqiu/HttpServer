#include "http/context.h"
#include "csr/result.hpp"
#include "servererrors.h"
#include "socket/io.h"

void Request::setContent(const std::string &s) {
  content = {s.begin(), s.end()};
}

void Request::setContent(std::vector<char> &&v) { content = std::move(v); }

void Response::setContent(const std::string &s) {
  content = {s.begin(), s.end()};
}

void Response::setContent(std::vector<char> &&v) { content = std::move(v); }

Context::Context(m_sock_t fd) : fd(fd) {}

void Context::write() {
  if (resp.headers.empty()) {
    return;
  }

  Writer writer{fd};

  // omit reason phrase here
  writer.write("HTTP/1.0 " + resp.status + " \r\n").unwrap();

  if (!resp.content.empty()) {
    resp.headers["Content-Length"] = std::to_string(resp.content.size());
  }
  for (const auto &[key, value] : resp.headers) {
    writer.write(key + ":" + value + "\r\n").unwrap();
  }

  writer.write("\r\n", 2).unwrap();

  if (!resp.content.empty()) {
    writer.write(resp.content).unwrap();
  }

  writer.flush().unwrap();
}
