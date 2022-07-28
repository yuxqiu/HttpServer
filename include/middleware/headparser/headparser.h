#pragma once

#include "http/context.h"
#include "http/task.h"
#include "socket/io.h"

class HeadParser {
private:
  size_t limit;

private:
  csr::Result<std::monostate, server_error_t> parse_req(Context &ctx,
                                                        Reader &reader) const;
  csr::Result<std::monostate, server_error_t>
  parse_header(Context &ctx, Reader &reader) const;

  csr::Result<std::monostate, server_error_t> parse(Context &ctx) const;

public:
  HeadParser();
  HeadParser(size_t limit);
  ~HeadParser() = default;
  HeadParser(const HeadParser &other) = default;
  HeadParser(HeadParser &&other) = default;

  HeadParser &operator=(const HeadParser &other) = delete;
  HeadParser &operator=(HeadParser &&other) = delete;

  void operator()(Context &ctx, const Task &next);
};