#pragma once
#include <netdb.h>

// RAII type for socket connection
class Socket {
 public:
  explicit Socket(int fd) : fd_(fd) {}

  Socket(Socket&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }

  ~Socket() {
    if (fd_ != -1) {
      close(fd_);
    }
  }

  [[nodiscard]] auto get() const -> int { return fd_; }

  auto operator=(Socket&& other) noexcept -> Socket& {
    fd_ = other.fd_;
    other.fd_ = -1;
    return *this;
  }

  Socket(Socket const& other) = delete;

  auto operator=(Socket const& other) -> Socket& = delete;

  auto operator==(Socket const& other) const noexcept -> bool { return fd_ == other.fd_; }

 private:
  int fd_;
};

// RAII wrapper for addrinfo
class AddrInfo {
 public:
  auto static make(char const* node, char const* service, addrinfo const* hints)
    -> std::expected<AddrInfo, int> {
    addrinfo* info = nullptr;
    int const status = getaddrinfo(node, service, hints, &info);
    if (status != 0) {
      return std::unexpected(status);
    }
    return AddrInfo{info};
  }

  ~AddrInfo() {
    if (info_ != nullptr) {
      freeaddrinfo(info_);
    }
  }

  // Non-copyable
  AddrInfo(AddrInfo const&) = delete;
  auto operator=(AddrInfo const&) -> AddrInfo& = delete;

  // Movable
  AddrInfo(AddrInfo&& other) noexcept : info_(other.info_) { other.info_ = nullptr; }

  auto operator=(AddrInfo&& other) noexcept -> AddrInfo& {
    if (this != &other) {
      info_ = other.info_;
      other.info_ = nullptr;
    }
    return *this;
  }
  auto operator->() const noexcept -> addrinfo* { return info_; }

 private:
  AddrInfo(addrinfo* info) : info_(info) {}
  addrinfo* info_{};
};
