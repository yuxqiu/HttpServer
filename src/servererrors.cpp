#include "servererrors.h"
#include <cstdio>

const char *ServerCategory::name() const noexcept { return "server"; }

std::string ServerCategory::message(int ev) const {
  switch (ev) {
  case ServerErr::max_len_reached:
    return "max length is reached";
  case ServerErr::no_available_address:
    return "no available address";
  case ServerErr::getaddrinfo_fail:
    return "getaddrinfo failed";
  case ServerErr::connection_close_by_client:
    return "connection closed by client";
  case ServerErr::invalid_request:
    return "invalid request format";
  case ServerErr::invalid_header:
    return "invalid header fields";
  case ServerErr::numeric_limit_reached:
    return "numeric limit reached";
  }
  return "unknown error";
}