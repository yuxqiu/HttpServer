#include "middleware/headparser/headparser.h"
#include <sstream>

const char *ws = " \t\n\r\f\v";
static inline std::string &trim(std::string &s, const char *t);

HeadParser::HeadParser() : limit(0) {}

HeadParser::HeadParser(size_t limit) : limit(limit) {}

void HeadParser::operator()(Context &ctx, const Task &next) {
  if (!parse(ctx).is_err()) {
    next.next(ctx);
  }
}

csr::Result<std::monostate, server_error_t>
HeadParser::parse(Context &ctx) const {
  if (limit) {
    LimitSizeReader reader{ctx.fd, limit};

    auto err = parse_req(ctx, reader);
    if (err.is_err()) {
      return err;
    }

    err = parse_header(ctx, reader);
    if (err.is_err()) {
      return err;
    }
  } else {
    Reader reader{ctx.fd};

    auto err = parse_req(ctx, reader);
    if (err.is_err()) {
      return err;
    }

    err = parse_header(ctx, reader);
    if (err.is_err()) {
      return err;
    }
  }

  return csr::Result<std::monostate, server_error_t>();
}

csr::Result<std::monostate, server_error_t>
HeadParser::parse_req(Context &ctx, Reader &reader) const {
  std::string s;
  auto read_result = reader.readline(s);

  if (read_result.is_err()) {
    return csr::Result<std::monostate, std::system_error>::Err(
        std::move(read_result.unwrap_err()));
  }
  if (read_result.unwrap() == 0) {
    return csr::Result<std::monostate, server_error_t>::Err(
        server_error(ServerErr::connection_close_by_client, "parse_req error"));
  }

  std::istringstream is{std::move(s)};

  if (!std::getline(is, ctx.req.method, ' ')) {
    return csr::Result<std::monostate, server_error_t>::Err(
        server_error(ServerErr::invalid_request, "parse_req error"));
  }

  if (!std::getline(is, ctx.req.fullpath, ' ')) {
    return csr::Result<std::monostate, server_error_t>::Err(
        server_error(ServerErr::invalid_request, "parse_req error"));
  }

  if (!std::getline(is, ctx.req.version, ' ')) {
    return csr::Result<std::monostate, server_error_t>::Err(
        server_error(ServerErr::invalid_request, "parse_req error"));
  }

  return csr::Result<std::monostate, std::system_error>();
}

csr::Result<std::monostate, server_error_t>
HeadParser::parse_header(Context &ctx, Reader &reader) const {
  std::string s;
  while (true) {
    auto read_result = reader.readline(s);
    if (read_result.is_err()) {
      return csr::Result<std::monostate, std::system_error>::Err(
          std::move(read_result.unwrap_err()));
    }
    if (read_result.unwrap() == 0) {
      return csr::Result<std::monostate, server_error_t>::Err(server_error(
          ServerErr::connection_close_by_client, "parse_req error"));
    }

    if (s == "\r\n") {
      break;
    }

    std::istringstream is{std::move(s)};
    if (!std::getline(is, s, ':')) {
      return csr::Result<std::monostate, server_error_t>::Err(
          server_error(ServerErr::invalid_header, "parse_header error"));
    }

    std::string value;
    if (!std::getline(is, value)) {
      return csr::Result<std::monostate, server_error_t>::Err(
          server_error(ServerErr::invalid_header, "parse_header error"));
    }

    trim(value, ws);
    ctx.req.headers.emplace(std::make_pair<std::string, std::string>(
        std::move(s), std::move(value)));

    s.clear();
  }

  return csr::Result<std::monostate, std::system_error>();
}

// trim from end of string (right)
static inline std::string &rtrim(std::string &s, const char *t = ws) {
  s.erase(s.find_last_not_of(t) + 1);
  return s;
}

// trim from beginning of string (left)
static inline std::string &ltrim(std::string &s, const char *t = ws) {
  s.erase(0, s.find_first_not_of(t));
  return s;
}

// trim from both ends of string (right then left)
static inline std::string &trim(std::string &s, const char *t = ws) {
  return ltrim(rtrim(s, t), t);
}