#pragma once

#include "../YgorMeshesBooleanBounded.h"
#include <variant>

namespace ygor::mesh_boolean::bounded {
template<class T>
class boolean_outcome {
  public:
    static boolean_outcome success(T value) { return boolean_outcome(std::move(value)); }
    static boolean_outcome failure(bounded_boolean_error error) { return boolean_outcome(std::move(error)); }
    bool has_value() const noexcept { return std::holds_alternative<T>(value_); }
    T *value() noexcept { return std::get_if<T>(&value_); }
    const T *value() const noexcept { return std::get_if<T>(&value_); }
    bounded_boolean_error *error() noexcept { return std::get_if<bounded_boolean_error>(&value_); }
    const bounded_boolean_error *error() const noexcept { return std::get_if<bounded_boolean_error>(&value_); }
  private:
    explicit boolean_outcome(T value) : value_(std::move(value)) {}
    explicit boolean_outcome(bounded_boolean_error error) : value_(std::move(error)) {}
    std::variant<T, bounded_boolean_error> value_;
};
template<>
class boolean_outcome<void> {
  public:
    static boolean_outcome success() { return boolean_outcome(true, {}); }
    static boolean_outcome failure(bounded_boolean_error error) { return boolean_outcome(false, std::move(error)); }
    bool has_value() const noexcept { return success_; }
    const bounded_boolean_error *error() const noexcept { return success_ ? nullptr : &error_; }
  private:
    boolean_outcome(bool success, bounded_boolean_error error) : success_(success), error_(std::move(error)) {}
    bool success_;
    bounded_boolean_error error_;
};
}
