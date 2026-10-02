#include "send.hpp"

#include <cassert>

[[nodiscard]] auto nc::send(Socket const& sock, std::string_view msg) -> std::optional<Errno> {
  assert(msg.size() < std::numeric_limits<uint32_t>::max());
  auto msg_size = static_cast<uint32_t>(msg.size());
  if (::send(sock.get(), &msg_size, sizeof(msg_size), 0) == -1) {
    return Errno{errno};
  }
  uint32_t total_bytes_sent = 0;
  while (total_bytes_sent < msg_size) {
    auto bytes_sent = ::send(sock.get(), msg.data(), msg_size, 0);
    if (bytes_sent == -1) {
      return Errno{errno};
    }
    total_bytes_sent += static_cast<uint32_t>(bytes_sent);
  }
  return std::nullopt;
}
