#include "socket/io.h"
#include <cstring>

#ifdef _WIN32
#include <limits>
#endif

Reader::Reader(m_sock_t connfd)
    : buffer(), usable_buf(buffer), fd(connfd), cnt(0) {}

/*
 *    This is a wrapper for the read()/send() function that
 *    transfers min(n, rio_cnt) bytes from an internal buffer to a user
 *    buffer, where n is the number of bytes requested by the user and
 *    rio_cnt is the number of unread bytes in the internal buffer. On
 *    entry, rio_read() refills the internal buffer via a call to
 *    read() if the internal buffer is empty.
 */
csr::Result<size_t, std::system_error> Reader::read(char *usrbuf, size_t len) {

  while (!cnt) {
#if defined(__APPLE__) || defined(__linux__)
    ssize_t rc;
    if (ISSOCKETERROR((rc = ::read(fd, buffer, sizeof(buffer))))) {
      if (GETSOCKETERRNO() != EINTR) {
        return csr::Result<size_t, std::system_error>::Err(
            sys_socket_error("read error"));
      }
    }
#elif defined(_WIN32)
    int rc;
    if (ISSOCKETERROR((rc = ::recv(fd, buffer, (int)sizeof(buffer), 0)))) {
      if (GETSOCKETERRNO() != WSAEINTR) {
        return csr::Result<size_t, std::system_error>::Err(
            sys_socket_error("read error"));
      }
    }
#endif
    else if (rc == 0) {
      return csr::Result<size_t, std::system_error>::Ok(0);
    } else {
      cnt = (size_t)rc;
    }
  }

  size_t bytes_to_copy = len < cnt ? len : cnt;
  memcpy(usrbuf, usable_buf, bytes_to_copy);

  usable_buf += bytes_to_copy;
  cnt -= bytes_to_copy;
  if (cnt == 0) {
    usable_buf = buffer;
  }

  return csr::Result<size_t, std::system_error>::Ok(std::move(bytes_to_copy));
}

/*
 *    Read n-1 characters via Unix syscall read().
 *    The null terminator is appended at the end.
 */
csr::Result<size_t, std::system_error> Reader::readn(char *usrbuf, size_t n) {
  size_t left = n - 1;
  while (left) {
    auto read_result = read(usrbuf, left);
    if (read_result.is_err()) {
      return read_result;
    }

    size_t rc = read_result.unwrap();
    if (rc == 0) {
      break;
    }

    left -= rc;
    usrbuf += rc;
  }
  *usrbuf = 0;

  return csr::Result<size_t, std::system_error>::Ok(n - left);
}

csr::Result<size_t, std::system_error> Reader::read_char(char *c) {
  return read(c, 1);
}

/*
 * Read a line into vector<char> usrbuf.
 * The newline character \n is included.
 */
csr::Result<size_t, std::system_error>
Reader::readline(std::vector<char> &usrbuf) {
  char c = 0;

  while (c != '\n') {
    auto read_char_result = read_char(&c);
    if (read_char_result.is_err()) {
      return read_char_result;
    }

    size_t rc = read_char_result.unwrap();
    if (rc == 0) {
      break;
    }
    usrbuf.push_back(std::move(c));
  }

  return csr::Result<size_t, std::system_error>::Ok(usrbuf.size());
}

csr::Result<size_t, std::system_error> Reader::readline(std::string &usrbuf) {
  char c = 0;

  while (c != '\n') {
    auto read_char_result = read_char(&c);
    if (read_char_result.is_err()) {
      return read_char_result;
    }

    size_t rc = read_char_result.unwrap();
    if (rc == 0) {
      break;
    }
    usrbuf.push_back(std::move(c));
  }

  return csr::Result<size_t, std::system_error>::Ok(usrbuf.size());
}

LimitSizeReader::LimitSizeReader(m_sock_t connfd, size_t maxlen)
    : Reader(connfd), maxlen(maxlen) {}

csr::Result<size_t, server_error_t> LimitSizeReader::read(char *usrbuf,
                                                          size_t n) {
  if (maxlen == 0) {
    return csr::Result<size_t, server_error_t>::Err(
        server_error(ServerErr::max_len_reached, "read error"));
  }

  auto read_result = Reader::read(usrbuf, maxlen < n ? maxlen : n);
  if (read_result.is_ok()) {
    maxlen -= read_result.unwrap();
  }
  return read_result;
}

Writer::Writer(m_sock_t connfd) : buffer(), cnt(0), fd(connfd) {}

csr::Result<size_t, std::system_error> Writer::write_ub(const char *usrbuf,
                                                        size_t size) const {
  size_t nleft = size;

  while (nleft) {
#if defined(__APPLE__) || defined(__linux__)
    ssize_t rc;
    if (ISSOCKETERROR((rc = ::write(fd, usrbuf, nleft)))) {
      if (GETSOCKETERRNO() != EINTR) {
        return csr::Result<size_t, std::system_error>::Err(
            sys_socket_error("write error"));
      }
      rc = 0;
    }
#elif defined(_WIN32)
    int rc;
    // windows send function requires nleft to be int
    if (nleft > (size_t)std::numeric_limits<int>::max()) {
      return csr::Result<size_t, std::system_error>::Err(
          server_error(ServerErr::numeric_limit_reached, "write_ub error"));
    }
    if (ISSOCKETERROR((rc = ::send(fd, usrbuf, (int)nleft, 0)))) {
      if (GETSOCKETERRNO() != WSAEINTR) {
        return csr::Result<size_t, std::system_error>::Err(
            sys_socket_error("write error"));
      }
      rc = 0;
    }
#endif

    nleft -= (size_t)rc;
    usrbuf += rc;
  }

  return csr::Result<size_t, std::system_error>::Ok(std::move(size));
}

csr::Result<size_t, std::system_error> Writer::flush() {
  auto write_result = write_ub(buffer, cnt);
  if (write_result.is_err()) {
    return write_result;
  }

  cnt = 0;
  return csr::Result<size_t, std::system_error>::Ok(sizeof(buffer));
}

csr::Result<size_t, std::system_error> Writer::write(const char *usrbuf,
                                                     size_t size) {
  size_t avail = (sizeof(buffer) - cnt), nleft = size;
  size_t bytes_to_write_to_buffer = nleft < avail ? nleft : avail;

  memcpy(buffer + cnt, usrbuf, bytes_to_write_to_buffer);
  nleft -= bytes_to_write_to_buffer;
  usrbuf += bytes_to_write_to_buffer;
  cnt += bytes_to_write_to_buffer;

  if (cnt == sizeof(buffer)) {
    auto flush_result = flush();
    if (flush_result.is_err()) {
      return flush_result;
    }
  }

  if (nleft) {
    if (nleft >= sizeof(buffer)) {
      auto write_result = write_ub(usrbuf, nleft);
      if (write_result.is_err()) {
        return write_result;
      }
    } else {
      memcpy(buffer, usrbuf, nleft);
    }
  }

  return csr::Result<size_t, std::system_error>::Ok(std::move(size));
}

csr::Result<size_t, std::system_error>
Writer::write(const std::vector<char> &usrbuf) {
  return write(usrbuf.data(), usrbuf.size());
}

csr::Result<size_t, std::system_error>
Writer::write(const std::string &usrbuf) {
  return write(usrbuf.c_str(), usrbuf.size());
}