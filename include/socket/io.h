#pragma once

#include "common.h"
#include "csr/result.hpp"
#include "servererrors.h"
#include "socket/socket_common.h"
#include <vector>

constexpr size_t BUFSIZE = 8192;

class Reader {
private:
  char buffer[BUFSIZE];
  char *usable_buf;
  m_sock_t fd;

protected:
  size_t cnt;

public:
  Reader(m_sock_t connfd);
  virtual ~Reader() = default;

  NOT_COPYABLE(Reader);
  NOT_MOVEABLE(Reader);

  virtual csr::Result<size_t, std::system_error> read(char *usrbuf, size_t n);
  csr::Result<size_t, std::system_error> readn(char *usrbuf, size_t n);
  csr::Result<size_t, std::system_error> read_char(char *c);
  csr::Result<size_t, std::system_error> readline(std::vector<char> &usrbuf);
  csr::Result<size_t, std::system_error> readline(std::string &usrbuf);
};

class LimitSizeReader : public Reader {
private:
  size_t maxlen;

public:
  LimitSizeReader(m_sock_t connfd, size_t maxlen);
  virtual ~LimitSizeReader() = default;

  NOT_COPYABLE(LimitSizeReader);
  NOT_MOVEABLE(LimitSizeReader);

  virtual csr::Result<size_t, server_error_t> read(char *usrbuf,
                                                   size_t n) override;
};

class Writer {
private:
  char buffer[BUFSIZE];
  size_t cnt;
  m_sock_t fd;

  csr::Result<size_t, std::system_error> write_ub(const char *usrbuf,
                                                  size_t size) const;

public:
  Writer(m_sock_t connfd);
  ~Writer() = default;

  NOT_COPYABLE(Writer);
  NOT_MOVEABLE(Writer);

  csr::Result<size_t, std::system_error> write(const char *usrbuf, size_t size);
  csr::Result<size_t, std::system_error> write(const std::vector<char> &usrbuf);
  csr::Result<size_t, std::system_error> write(const std::string &usrbuf);

  csr::Result<size_t, std::system_error> flush();
};