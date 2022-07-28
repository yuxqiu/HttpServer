#pragma once

#include "csr/option.hpp"
#include "csr/result.hpp"
#include "servererrors.h"
#include "socket/io.h"
#include "socket/socket_common.h"
#include <variant>

class SocketClient {
private:
  SocketClient(m_sock_t connfd, socklen_t clientlen,
               const struct sockaddr_storage &clientaddr);

public:
  csr::Option<m_sock_t> connfd;
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

public:
  SocketClient(SocketClient &&other);
  ~SocketClient();

  SocketClient &operator=(SocketClient &&other) = delete;
  NOT_COPYABLE(SocketClient);

  friend class Socket;
};

class Socket {
private:
  csr::Option<m_sock_t> sockfd;

private:
  Socket(m_sock_t sockfd);

  static csr::Result<m_sock_t, std::system_error>
  _Accept(m_sock_t listenfd, struct sockaddr *addr, socklen_t *addrlen);

public:
  Socket(Socket &&other);
  ~Socket();

  Socket &operator=(Socket &&other) = delete;
  NOT_COPYABLE(Socket);

  csr::Result<SocketClient, std::system_error> accept() const;

  friend class SocketGenerator;
};

class SocketGenerator {
private:
  static csr::Result<std::monostate, std::system_error>
  _Getaddrinfo(const char *node, const char *service,
               const struct addrinfo *hints, struct addrinfo **res);

  static csr::Result<m_sock_t, std::system_error> _Socket(int domain, int type,
                                                          int protocol);
  static csr::Result<std::monostate, std::system_error>
  _Bind(m_sock_t sockfd, const struct sockaddr *addr, socklen_t addrlen);
  static csr::Result<std::monostate, std::system_error> _Listen(m_sock_t sockfd,
                                                                int backlog);

public:
  static csr::Result<Socket, server_error_t> listen(int port);
};