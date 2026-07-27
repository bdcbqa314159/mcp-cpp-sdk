#pragma once
#include <mcp/json_rpc.hpp>
#include <stdexcept>
#include <utility>
#include <variant>

namespace mcp {

// Result<T> — a value OR an error, our C++20 stand-in for std::expected<T, Error>.
// This is "error-as-value": instead of throwing, a function returns Result<T>,
// and the caller checks ok() before reading value(). (At C++23 we can alias this
// to std::expected with little churn.)
//
// It's a CLASS TEMPLATE: `Result<int>`, `Result<Response>`, etc. all instantiate
// from this one definition. Template member functions live in the header.
template <typename T> class Result {
public:
  // Two ways to build one. Both non-explicit so `return value;` / `return err;`
  // just work from a function that returns Result<T>.
  Result(T value) : data_(std::move(value)) {}     // success
  Result(Error error) : data_(std::move(error)) {} // failure

  // TODO (task): true when data_ currently holds a T (not an Error).
  //   Tool: std::holds_alternative<T>(data_).
  [[nodiscard]] bool ok() const { return std::holds_alternative<T>(data_); }

  // Access the success value. Precondition: ok() == true.
  // TODO: return the T stored in data_  (std::get<T>(data_)).
  T& value() {
    if (this->ok())
      return std::get<T>(data_);
    throw std::logic_error("Result::value() on an error");
  }
  const T& value() const {
    if (this->ok())
      return std::get<T>(data_);
    throw std::logic_error("Result::value() on an error");
  }

  // Access the error. Precondition: ok() == false.
  // TODO: return the Error stored in data_  (std::get<Error>(data_)).
  [[nodiscard]] const Error& error() const {
    if (!this->ok())
      return std::get<Error>(data_);
    throw std::logic_error("Result::error() on an success");
  }

private:
  std::variant<T, Error> data_;
};

} // namespace mcp
