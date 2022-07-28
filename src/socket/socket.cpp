#include "socket/socket.h"
#include <cerrno>
#include <cstring>

#ifdef _WIN32
#include <limits>
#endif

constexpr size_t LISTENQ = 1024;

static csr::Result<std::monostate, std::system_error> Close(m_sock_t fd);

#ifdef _WIN32
static csr::Result<std::monostate, std::system_error> init_WSA();
static csr::Result<std::monostate, std::system_error> cleanup_WSA();
#endif

SocketClient::SocketClient(m_sock_t connfd, socklen_t clientlen,
                           const struct sockaddr_storage &clientaddr)
    : connfd(csr::Option<m_sock_t>::Some(std::move(connfd))),
      clientlen(clientlen), clientaddr(clientaddr) {}

SocketClient::~SocketClient() {
  if (connfd.is_some()) {
    Close(connfd.unwrap()).unwrap();
  }
}

SocketClient::SocketClient(SocketClient &&other)
    : connfd(std::move(other.connfd)), clientlen(other.clientlen),
      clientaddr(other.clientaddr) {
  other.connfd = csr::Option<int>::None();
}

Socket::Socket(m_sock_t sockfd)
    : sockfd(csr::Option<m_sock_t>::Some(std::move(sockfd))) {}

// * This destructor may throw exception
Socket::~Socket() {
  if (sockfd.is_some()) {
    Close(sockfd.unwrap()).unwrap();

#ifdef _WIN32
    cleanup_WSA().unwrap();
#endif
  }
}

Socket::Socket(Socket &&other) : sockfd(std::move(other.sockfd)) {
  other.sockfd = csr::Option<int>::None();
}

csr::Result<m_sock_t, std::system_error>
Socket::_Accept(m_sock_t listenfd, struct sockaddr *addr, socklen_t *addrlen) {
  m_sock_t fd;
  if (ISINVALIDSOCKET((fd = ::accept(listenfd, addr, addrlen)))) {
    return csr::Result<int, std::system_error>::Err(
        sys_socket_error("accept error"));
  }
  return csr::Result<m_sock_t, std::system_error>::Ok(std::move(fd));
  ;
}

csr::Result<SocketClient, std::system_error> Socket::accept() const {
  struct sockaddr_storage clientaddr;
  socklen_t clientlen = sizeof(struct sockaddr_storage);

  auto accept_ret =
      _Accept(sockfd.unwrap(), (struct sockaddr *)&clientaddr, &clientlen);
  if (accept_ret.is_err()) {
    return csr::Result<SocketClient, std::system_error>::Err(
        std::move(accept_ret.unwrap_err()));
  }

  return csr::Result<SocketClient, std::system_error>::Ok(
      SocketClient{accept_ret.unwrap(), clientlen, clientaddr});
}

csr::Result<std::monostate, std::system_error>
SocketGenerator::_Getaddrinfo(const char *node, const char *service,
                              const struct addrinfo *hints,
                              struct addrinfo **res) {
  int rc;
  if ((rc = getaddrinfo(node, service, hints, res)) != 0) {
    // use server_error to encapsulate the getaddrinfo error
    // as this error is not supported by standard Unix errno
    return csr::Result<std::monostate, std::system_error>::Err(
        server_error(ServerErr::getaddrinfo_fail,
                     "getaddrinfo error(" + std::to_string(rc) + ")"));
  }
  return csr::Result<std::monostate, std::system_error>();
}

csr::Result<m_sock_t, std::system_error>
SocketGenerator::_Socket(int domain, int type, int protocol) {
  m_sock_t fd;
  if (ISINVALIDSOCKET((fd = socket(domain, type, protocol)))) {
    return csr::Result<int, std::system_error>::Err(
        sys_socket_error("close error"));
  }
  return csr::Result<m_sock_t, std::system_error>::Ok(std::move(fd));
}

csr::Result<std::monostate, std::system_error>
SocketGenerator::_Bind(m_sock_t sockfd, const struct sockaddr *addr,
                       socklen_t addrlen) {
  if (ISSOCKETERROR((bind(sockfd, addr, addrlen)))) {
    return csr::Result<std::monostate, std::system_error>::Err(
        sys_socket_error("bind error"));
  }
  return csr::Result<std::monostate, std::system_error>();
}

csr::Result<std::monostate, std::system_error>
SocketGenerator::_Listen(m_sock_t sockfd, int backlog) {
  if (ISSOCKETERROR((::listen(sockfd, backlog)))) {
    return csr::Result<std::monostate, std::system_error>::Err(
        sys_socket_error("listen error"));
  }
  return csr::Result<std::monostate, std::system_error>();
}

csr::Result<Socket, server_error_t> SocketGenerator::listen(int port) {
#ifdef _WIN32
  auto startup_result = init_WSA();
  if (startup_result.is_err()) {
    return csr::Result<Socket, std::system_error>::Err(
        std::move(startup_result.unwrap_err()));
  }
#endif

  struct addrinfo hints;
  struct addrinfo *res, *p;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_socktype = SOCK_STREAM; /* Accept connections */
  hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
  hints.ai_flags |= AI_NUMERICSERV; /* ... using port number */

  auto getaddrinfo_result =
      _Getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &res);
  if (getaddrinfo_result.is_err()) {
#ifdef _WIN32
    cleanup_WSA().unwrap();
#endif
    return csr::Result<Socket, std::system_error>::Err(
        std::move(getaddrinfo_result.unwrap_err()));
  }

  m_sock_t listenfd;

  for (p = res; p; p = p->ai_next) {
    auto socket_ret = _Socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (socket_ret.is_err()) {
      continue;
    }

    listenfd = socket_ret.unwrap();

#if defined(_WIN32)
    // it's reasonable to assume that ai_addrlen is smaller than int max
    // however, it's better to add a test to ensure this never happen
    if (p->ai_addrlen > (size_t)std::numeric_limits<int>::max()) {
      return csr::Result<Socket, std::system_error>::Err(
          server_error(ServerErr::numeric_limit_reached, "listen error"));
    }
#endif

    /* Bind the descriptor to the address */
    if (_Bind(listenfd, p->ai_addr, (socklen_t)p->ai_addrlen).is_ok())
      break;

    Close(listenfd).unwrap();
  }

  freeaddrinfo(res);

  if (!p) {
#ifdef _WIN32
    cleanup_WSA().unwrap();
#endif
    return csr::Result<Socket, std::system_error>::Err(
        server_error(ServerErr::no_available_address, "listen error"));
  }

  /* Make it a listening socket ready to accept connection requests */
  auto listen_ret = _Listen(listenfd, LISTENQ);
  if (listen_ret.is_err()) {
#ifdef _WIN32
    cleanup_WSA().unwrap();
#endif
    return csr::Result<Socket, std::system_error>::Err(
        std::move(listen_ret.unwrap_err()));
  }

  return csr::Result<Socket, std::system_error>::Ok(Socket{listenfd});
}

static csr::Result<std::monostate, std::system_error> Close(m_sock_t fd) {
  int rc;

#if defined(__APPLE__) || defined(__linux__)
  if (ISSOCKETERROR((rc = close(fd)))) {
    return csr::Result<std::monostate, std::system_error>::Err(
        sys_socket_error("close error"));
  }
#elif defined(_WIN32)
  if (ISSOCKETERROR((rc = closesocket(fd)))) {
    return csr::Result<std::monostate, std::system_error>::Err(
        sys_socket_error("close error"));
  }
#endif

  return csr::Result<std::monostate, std::system_error>();
}

#ifdef _WIN32
static csr::Result<std::monostate, std::system_error> init_WSA() {
  WSADATA wsaData;
  int ec_startup;

  // Initialize Winsock
  ec_startup = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (ec_startup != 0) {
    // error is directly returned, so avoid using macro
    return csr::Result<std::monostate, std::system_error>::Err(
        std::system_error(ec_startup, std::system_category(),
                          "WSAStartup error"));
  }

  return csr::Result<std::monostate, std::system_error>();
}

static csr::Result<std::monostate, std::system_error> cleanup_WSA() {
  int ec_cleanup;
  ec_cleanup = WSACleanup();
  if (ec_cleanup != 0) {
    return csr::Result<std::monostate, std::system_error>::Err(
        sys_socket_error("WSACleanup error"));
  }
  return csr::Result<std::monostate, std::system_error>();
}
#endif