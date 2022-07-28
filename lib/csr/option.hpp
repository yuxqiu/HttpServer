/*
 * option.hpp
 *
 * Created by Yuxiang Qiu
 *
 * Repository: https://github.com/yuxqiu/cpp-as-rust
 */

#pragma once

#include <cstdio>
#include <optional>
#include <string>
#include <utility>

namespace csr {
template <typename T> class Option {
private:
  std::optional<T> option;

private:
  Option(T &&val);
  Option(std::optional<T> &&other);

  template <typename X> Option process(Option<X> &&other);

public:
  Option(Option<T> &&other);

  template <typename X> Option(Option<X> &&other);

  Option &operator=(Option<T> &&other);
  ~Option() = default;
  Option() = delete;
  Option(const Option<T> &_) = delete;
  Option &operator=(const Option<T> &_) = delete;

  bool is_some() const;
  bool is_none() const;

  // Observer
  const T &expect(const std::string &message) const;
  T &expect(const std::string &message);
  const T &unwrap() const;
  T &unwrap();

  T unwrap_or(T &&default_value);

  inline static Option<T> Some(T &&x);
  inline static Option<T> None();
};

template <typename T> Option<T>::Option(T &&val) : option(std::move(val)) {}

template <typename T>
Option<T>::Option(std::optional<T> &&other) : option(std::move(other)) {}

template <typename T>
Option<T>::Option(Option<T> &&other) : option(std::move(other.option)) {}

template <typename T>
template <typename X>
Option<T>::Option(Option<X> &&other) : Option(process(std::move(other))) {}

template <typename T> Option<T> &Option<T>::operator=(Option<T> &&other) {
  option = std::move(other.option);
  return *this;
}

template <typename T>
template <typename X>
Option<T> Option<T>::process(Option<X> &&other) {
  if (other.is_some()) {
    return Option<T>::Some(std::move(other.unwrap()));
  }
  return Option<T>::None();
}

template <typename T> bool Option<T>::is_some() const {
  return option.has_value();
}

template <typename T> bool Option<T>::is_none() const { return !is_some(); }

template <typename T>
const T &Option<T>::expect(const std::string &message) const {
  if (is_none()) {
    fprintf(stderr, "%s\n", message.c_str());
  }
  return option.value();
}

template <typename T> T &Option<T>::expect(const std::string &message) {
  if (is_none()) {
    fprintf(stderr, "%s\n", message.c_str());
  }
  return option.value();
}

template <typename T> const T &Option<T>::unwrap() const {
  return option.value();
}

template <typename T> T &Option<T>::unwrap() { return option.value(); }

template <typename T> T Option<T>::unwrap_or(T &&default_value) {
  return is_some() ? unwrap() : default_value;
}

template <typename T> inline Option<T> Option<T>::Some(T &&x) {
  return Option<T>(std::move(x));
}

template <typename T> inline Option<T> Option<T>::None() {
  return Option<T>(std::nullopt);
}
} // namespace csr
