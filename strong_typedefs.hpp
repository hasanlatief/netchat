#ifndef NETCHAT_STRONG_TYPEDEF_HPP
#define NETCHAT_STRONG_TYPEDEF_HPP
#include <cstring>

template <typename Contained>
struct StrongTypedef {

  template <typename Self>
  [[nodiscard]] auto get(this Self&& self) noexcept -> decltype(auto) {
    return std::forward<Self>(self).value_;
  }

 protected:
  StrongTypedef(Contained value) : value_(std::move(value)) {}

 private:
  Contained value_;
};

struct Errno : StrongTypedef<error_t> {
  Errno(error_t err) : StrongTypedef(err), err_msg_{strerror(err)} {
    if (message().starts_with("Unknown error")) {
      throw std::out_of_range("Invalid errno value: " + std::to_string(err));
    }
  }
  [[nodiscard]] auto message() const -> std::string_view { return err_msg_; }

 private:
  std::string err_msg_;
};

#endif // NETCHAT_STRONG_TYPEDEF_HPP
