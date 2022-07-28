#pragma once

#include <string>
#include <system_error>

enum ServerErr : int {
  // Network related
  max_len_reached = 1,
  no_available_address,
  getaddrinfo_fail,
  connection_close_by_client,

  // Parser related
  invalid_request,
  invalid_header,

  // Other
  numeric_limit_reached,
};

class ServerCategory : public std::error_category {
public:
  virtual const char *name() const noexcept override;
  virtual std::string message(int ev) const override;
};

#define sys_socket_error(str)                                                  \
  std::system_error(GETSOCKETERRNO(), std::system_category(), str)
#define server_error(err, str) std::system_error(err, ServerCategory(), str)

// server_error_t = std::system_error + ServerErr
typedef std::system_error server_error_t;